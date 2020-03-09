#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

def determine_args(params):
    args = []
    for p in params:
        arg = {}
        arg['name'] = p.name
        arg['type'] = p.type().unparse().strip()
        if "[" in arg['type'].split()[-1] and "]" in arg['type'].split()[-1]: 
            arg['string'] = ' '.join(arg['type'].split()[:-1]) + " *" + arg['name'] #+ arg['type'].split()[-1]
        else:
            arg['string'] = arg['type'] + " " + arg['name']
        args.append(arg)
    return args


def get_called_funcs(project, func, funcs_to_copy_over):
    called_funcs = func.query("cf:ExprCall")
    for row in called_funcs:
        new_func = row.cf
        if new_func.name() not in [f.name for f in funcs_to_copy_over]:
            function_table = project.query("g:Global => f:FnDef", where="f.name == '%s'" % new_func.name())
            if len(function_table) == 0:
                continue
            for row in function_table:
                function = row.f
                if function and function.in_code():
                    funcs_to_copy_over.append(function)
                    get_called_funcs(project, function,funcs_to_copy_over)

def func_spec_pragma(func_name, args):
    pragma = "#pragma artisan-hls spec " + func_name + " "
    for a in args:
        if "*" in a['type']:
            if not '{' in pragma:
                pragma += " { "
            else:
                pragma += ", "
            pragma += '"' + a['name'] + '" : "OCL_ADDRSP_GLOBAL"'
    if '{' in pragma:
        pragma += " }"
    else:
        return False
    return pragma

# ********************************************fm************************************
# copy specified hotspot function into C++ kernel file 
# **********************************************************************************

# TODO: copy over all struct, type, and global var definiitions 
def populate_cpp_kernel(project, function, path_to_file, args):

    pragmas = ["#pragma artisan-hls component lib_func\n"]
    global_vars = [] # TODO
    structs_and_types = []  # TODO
    func_prototypes = []
    functions = []
    
    # get hotspot function in input program, unparse body and determine spec pragma
    hotspot_func = project.query("g:Global => f:FnDef{%s}" % function)[0].f
    lib_func_string = hotspot_func.unparse().replace('hotspot', 'lib_func')
    pragmas.append(func_spec_pragma('lib_func', args))
    
    # recursively find all functions called in hotspot
    funcs_to_copy_over = []
    get_called_funcs(project, hotspot_func, funcs_to_copy_over)
    for func in funcs_to_copy_over:
        args = determine_args(func.decl().params())
        func_decl = func.decl().return_type().unparse().strip() + " " + func.name + "(" + ', '.join([a['string'] for a in args]) + ")"
        func_spec = func_spec_pragma(func.name, args)
        if func_spec != False:
            pragmas.append(func_spec_pragma(func.name, args))
        func_prototypes.append(func_decl + ";")
        functions.append(func_decl + "\n" + func.body().unparse())

    cpp_file_str = "#include <cmath>\n#include <cstdint>\n"
    for i in pragmas:
        cpp_file_str += i + "\n"
    for i in global_vars:
        cpp_file_str += i + "\n"
    for i in structs_and_types:
        cpp_file_str += i + "\n"   
    for i in func_prototypes:
        cpp_file_str += i + "\n"
    for i in functions:
        cpp_file_str += i + "\n"
    cpp_file_str += lib_func_string

    with open(path_to_file, 'w') as f:
        f.write(cpp_file_str)