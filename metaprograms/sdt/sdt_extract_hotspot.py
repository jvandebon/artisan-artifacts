#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
import sys, subprocess, json


def get_program_constants(ast):
    constants = {}
    constant_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'constant' in row.p.directive()]
    for row in constant_pragmas:
        directive = row.p.directive()
        const = json.loads(' '.join(directive.split()[directive.split().index('constant') + 1:]))
        for c in const:
            constants[c] = const[c]
    return constants

def get_local_vars(loop):
    local_vars = []
    vardefs = loop.query("vd:VarDef")
    for row in vardefs:
        local_vars.append(row.vd.name)
    return local_vars

''' returns parameter list for calling extracted function and argument list for function definition'''
def get_referenced_vars(loop, local_vars, constants):
    # check all statements/expressions for references to variables that are not constants or local vars
    loop_exprs = loop.query("e:Expr")
    params = []
    arguments = []
    for row in loop_exprs:
        if len(row.e.children()) == 0 and row.e.parent().sg_type_real != 'SgFunctionCallExp' and not row.e.is_val():  # variable
            var = row.e.unparse().strip()
            if var not in local_vars and var not in constants and var not in params:
                if row.e.parent().sg_type_real == 'SgDotExp':  # handle dot expressions: a.x 
                    if var == row.e.parent().unparse().strip().split('.')[0].strip():
                        params.append(var)
                    elif row.e.sg_type_real == "SgVarRefExp":  # TODO: make sure this works for all 
                        continue
                    else:
                        params.append(row.e.parent().unparse().strip())
                        row.e.parent().instrument(pos='replace', code=row.e.unparse())
                else:
                    params.append(var)

                type_str = row.e.type().unparse().strip()
                if '[' in type_str and ']' in type_str: # TODO: handle arrays better
                    arguments.append(type_str.split('[')[0].strip() + " " + var + '[' + '['.join(type_str.split('[')[1:]).strip())
                else:
                    arguments.append(type_str.strip() + " " + var)

    return params, arguments


def get_extra_var_decls(parent_function, loop, params, arguments):
    # check expressions in function but not in loop 
    all_func_exprs = parent_function.query("e:Expr")
    loop_exprs = loop.query("e:Expr")
    func_exprs = []
    for row in all_func_exprs:
        if row.e.unparse().strip() not in [r.e.unparse().strip() for r in loop_exprs]:
            func_exprs.append(row.e)

    # find any variable definitions that are currently loop params
    vardefs = parent_function.query("vd:VarDef")
    function_body = loop.unparse()
    for row in vardefs:
        if row.vd.unparse().strip() in params:
            parent = row.vd.parent().unparse().strip()
            p = row.vd.unparse().strip()
            # if varable is not used outside of loop, add to hotspot func body and remove param 
            used = False
            for e in func_exprs:
                if p in e.unparse():
                    used = True
                    break
            if not used:
                function_body = row.vd.parent().unparse().strip() + "\n" + function_body
                idx = params.index(p)
                del params[idx]
                del arguments[idx]
                row.vd.parent().instrument(pos="replace", code="")
    
    return function_body



# inputs: ast, loop to be extracted, function 
def extract_hotspot_loop_to_function(ast, loop_tag, parent_func_tag):

    project = ast.project

    # get loop and parent function 
    parent_function = project.query("f:FnDef{%s}" % parent_func_tag)[0].f
    try: 
        loop = project.query("l:ForLoop{%s}" % loop_tag)[0].l
    except:
        loop = project.query("l:WhileLoop{%s}" % loop_tag)[0].l

    # determine function arguments and call parameters
    constants = get_program_constants(ast)
    local_vars = get_local_vars(loop) 
    params, arguments = get_referenced_vars(loop, local_vars, constants)

    # if a param is declared but not used before the loop body, move declaration into function (not param)
    function_body = get_extra_var_decls(parent_function, loop, params, arguments)

    # create new function containing loop body with determined args
    hotspot_function_string = "void hotspot(" + ', '.join(arguments) + ") {\n " + function_body +"\n}"
    parent_function.instrument(pos='before', code=hotspot_function_string)

    # replace loop body in source with call to the new function
    loop.instrument(pos='replace', code="hotspot(" + ', '.join(params)+ ");\n")
    ast.commit()
    ast.export_to("extracted-hotspot")

# ast = model(args=cli(), ws=Workspace('tmp'))
# extract_hotspot_loop_to_function(ast, 'main_for_e', 'main')
# subprocess.call(['rm', '-rf', 'tmp'])
