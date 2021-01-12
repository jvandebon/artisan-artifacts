#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *


def initialise_array(arg_name, param):

    ## TODO: only 1D now
    size = arg_name[arg_name.index('[')+1:arg_name.index(']')]
    name = arg_name[:arg_name.index('[')]
    string = "for (int " + name + "_idx = 0; " + name + "_idx < " + size + "; " + name + "_idx++){\n"
    string+= name + "[" + name + "_idx ] = " + param + "[" + name + "_idx];\n}\n"
    return string


def inline_function(ast, func_tag, parent_func_tag):

    project = ast.project

    # find function
    func_to_inline = project.query('f:FnDef{%s}' % func_tag)[0].f
    parent_func = project.query('f:FnDef{%s}' % parent_func_tag)[0].f
   
    # change all local variable names in func_to_inline to avoid clashes (USE BFS)
    toVisit = []
    for c in func_to_inline.children():
        if not c == None:
            toVisit.append(c)
    for node in toVisit:
        if node.entity == 'VarDef':
            var = node.unparse().strip()
            decl = node.parent().unparse()
            new = decl.replace(" "+var, " "+func_tag+'_'+var)
            node.parent().instrument(pos='replace', code=new)
            ## BUG: ISSUES WTH FOR LOOP INIT STATEMENT UNPARSING
            parentparent = node.parent().parent().unparse().strip()
            if parentparent[-1] == ';' and parentparent[-2] == '\n':
                node.parent().parent().instrument(pos='replace', code=parentparent[:-2])
        if node.sg_type_real == 'SgVarRefExp':
            node.instrument(pos='before', code=func_tag+'_')
            ## BUG: issues instrumenting is part of a var def 
              
        for c in node.children():
            if not c == None:
                toVisit.append(c)


    
    # find call to function in parent  ## TODO: ONLY HANDLES ONE CALL NOW
    calls = parent_func.query('c:ExprCall')
    call = [row.c for row in calls if row.c.name() == func_tag][0]

    # check arg names, change var names to match call params in func body
    func_to_inline_decl = project.query('f:FnDecl{%s}' % func_tag)[0].f
    func_args = func_to_inline_decl.unparse()[func_to_inline_decl.unparse().index('(')+1:func_to_inline_decl.unparse().index(')')].split(',')
    
    call_string = call.unparse().strip()
    func_string = func_to_inline.body().unparse().strip()
    call_params = call_string[call_string.index('(')+1:call_string.index(')')].split(',')
    
    var_inits = ""
    
    func_body = func_to_inline.body().unparse()

    vars_to_replace = {}
    for i in range(len(call_params)):
        if '*' in func_args[i]:
            arg_type = func_args[i][:func_args[i].index('*')+1]
            arg_name = func_tag+"_"+(func_args[i][func_args[i].index('*')+1:]).replace(" ", "")
            # replace all instances of arg_name with call_params[i] to avoid changing address space
            vars_to_replace[arg_name] = '('+call_params[i]+')'    
        else:
            arg_name = func_tag+"_"+func_args[i].split()[-1]
            arg_type = func_args[i][:func_args[i].index(func_args[i].split()[-1])]

            param = call_params[i]
            if '[' in arg_name and ']' in arg_name:
                var_inits += arg_type + " " + arg_name + ";\n"
                # TODO: deal better with this
                var_inits += initialise_array(arg_name, param)    
            else:
                var_inits += arg_type + " " + arg_name + ";\n" + arg_name + " = " + param + ";\n"

    instrument_block(func_to_inline.body(), var_inits, entry=True)
    # print(func_to_inline.body().unparse().strip())

    # check result for return val (if any)
    call_string = call.parent().unparse()
    if '=' in call_string[:call_string.index('(')]:
        result = call_string[:call_string.index('=')+1]
    else:
        result = ""

    # change return statements to result assignment 
    # find return statement if any, remove and track variable for later 
    ret_stmts = func_to_inline.query('r:Return')
    if len(ret_stmts) > 0:
        for row in ret_stmts:
            ret_stmt = row.r
            if result == "":
                # remove void return 
                ret_stmt.instrument(pos='replace', code="") 
            else:
                ret_stmt.instrument(pos='replace', code=ret_stmt.unparse().replace("return", result))
                

    # replace call in parent with new function body 
    to_replace_call = func_to_inline.body().unparse()[1:func_to_inline.body().unparse().rfind('}')]
    for v in vars_to_replace:
        to_replace_call = to_replace_call.replace(v, vars_to_replace[v])
    call.parent().instrument(pos='replace', code=to_replace_call)
    func_to_inline.instrument(pos='replace', code="")
    ast.commit()

    
    


