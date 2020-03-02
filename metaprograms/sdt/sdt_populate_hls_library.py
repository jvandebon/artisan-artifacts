#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
log.level = 2


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
            arg['string'] = arg['type'] + " " + arg['name']
        args.append(arg)
    return args

def format_args(args):
    formatted_args = []
    for arg in args:
        if "*" in arg['string']:
            formatted_args.append("OCL_ADDRSP_GLOBAL " + arg['string'])
        else:
            formatted_args.append(arg['string'])
    return ', '.join(formatted_args)


def get_called_funcs(project, func, funcs_to_copy_over):
    called_funcs = func.query("cf:Call")
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

# ********************************************fm************************************
# copy hotspot function into C++ library file (device/lib/library.cpp)
# **********************************************************************************
# TODO: copy over all struct and constant definitions / create a common header file? 
def populate_hls_library(project, path_to_file, args):
    # get hotspot function in input program
     
    hotspot_func = project.query("g:Global => f:FnDef", where="f.name == 'hotspot'")[0].f
    
    # determine args and types 
    # TODO: now, all  pointer types are 'global'
    func_args = format_args(args)
    
    # function body string
    # TODO: be careful with  replace std::, check function 
    func_body = hotspot_func.body().unparse().replace('std::', '')
    
    # recursively find all functions called in hotspot
    funcs_to_copy_over = []
    get_called_funcs(project, hotspot_func, funcs_to_copy_over)
    functions = ""
    function_prototypes = ""
    for func in funcs_to_copy_over:
        args = format_args(determine_args(func.decl().params()))
        func_decl = func.decl().return_type().unparse().strip() + " " + func.name + "(" + args + ")"
        function_prototypes += func_decl + ";\n"
         # TODO: be careful with  replace std::, check function 
        functions += func_decl + "\n" + func.body().unparse().replace('std::', '')


    # write to template library file 
    with open(path_to_file, 'r') as f:
        f_contents = f.read()
    
    if "*FUNC_ARGS*" in f_contents:
        f_contents = f_contents.replace("*FUNC_ARGS*", func_args)
    if "*FUNC_BODY*" in f_contents:
        f_contents = f_contents.replace("*FUNC_BODY*", func_body)
    if "*OTHER_FUNCS*" in f_contents:
        f_contents = f_contents.replace("*OTHER_FUNCS*", function_prototypes + "\n" + functions)
    
    with open(path_to_file, 'w') as f:
        f.write(f_contents)

    return func_args