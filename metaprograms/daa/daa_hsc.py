#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
from hls_math_functions import hls_math_functions

def get_called_funcs(project, func, funcs_called):
    called_funcs = func.query("cf:ExprCall")
    for row in called_funcs:
        new_func = row.cf
        if new_func.name() not in [f.name for f in funcs_called]:
            function_table = project.query("g:Global => f:FnDef", where="f.name == '%s'" % new_func.name())
            for row in function_table:
                if row.f.in_code():
                    funcs_called.append(row.f)
                    get_called_funcs(project, row.f, funcs_called)

def hw_synthesizable(ast, func_name):

    project = ast.project
    func = project.query('f:FnDef{%s}' % func_name)[0].f
    funcs_to_check = [] 
    get_called_funcs(project, func, funcs_to_check)
    funcs_to_check.append(func)

    for func in funcs_to_check:   
        # check for calls to unsupported functions 
        all_calls = func.query('c:ExprCall')
        for row in all_calls:
            called_func = row.c
            if called_func.name() == "":
                # function pointer, not supported
                print("Function pointers are not supported for synthesis.")
                return False
            # check if defined in source 
            function_table = project.query("f:FnDef{%s}" % called_func.name())
            if not (len(function_table) != 0 and function_table[0].f.in_code()) and called_func.name() not in hls_math_functions:
                print("%s is not an OpenCL supported function." % called_func.name())
                return False
        
        # check for function pointers in function parameters and expressions
        for p in func.decl().params():
            type_str = p.type().unparse()
            if '(' in type_str and ')' in type_str:
                print("Function pointers are not supported for synthesis.")
                return False  

        exprs = func.query("e:Expr")
        bad_parent_types = ['SgFunctionCallExp', 'SgDotExp']
        for row in exprs:
            if len(row.e.children()) == 0:
                if not row.e.is_val() and not row.e.parent().sg_type_real in bad_parent_types:
                    type_str = row.e.type().unparse()
                    # TODO: beter way to check function pointer type?
                    if '(' in type_str and ')' in type_str:
                        print("Function pointers are not supported for HW synthesis.")
                        return False

        # check for recursion 
        funcs_called = []
        get_called_funcs(project, func, funcs_called)
        for fc in funcs_called:
            if func.decl().unparse() == fc.decl().unparse():
                print("Recursion is not supported for HW synthesis (%s)." % func.name)
                return False
    
    return True

# ast = model(args=cli())
# hw_synthesizable(ast,'hotspot')