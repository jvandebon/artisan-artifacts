#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import json, subprocess

# **********************************************************************************
# generate HLS kernel from C++ kernel
# **********************************************************************************

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



def generate_hls_kernel(project, path_to_hls_kernel):
    
    # find all pragma statements
    pragmas = project.query('g:Global => p:Pragma')
    component = ""
    specs = {}
    for row in pragmas:
        if row.p.in_code():
            directive = row.p.directive()
            if 'artisan-hls component' in directive:
                idx = directive.split().index('component') + 1
                component = directive.split()[idx]
            if 'artisan-hls spec' in directive:
                func_idx = directive.split().index('spec') + 1 
                func = directive.split()[func_idx]
                spec = json.loads(' '.join(directive.split()[func_idx+1:]))
                specs[func] = spec

    if component ==  "":
        print("No HLS component function specified.")
        exit(0)

    # TODO: find all struct/type definitions (write)
    # TODO: find all global var defitions (write)
    
    func_prototypes = []
    functions = []
    component_func_string = ""

    # find all functions
    function_table = project.query("g:Global => f:FnDef")
    for row in function_table:
        if row.f.in_code():
            args = determine_args(row.f.decl().params())
            # check if there is an associated spec pragma
            if row.f.name in specs:
                # if so, set up args as specified, unparse, set up prototype
                arg_str = []
                for arg in args:
                    if arg['name'] in specs[row.f.name]:
                        arg_str.append(specs[row.f.name][arg['name']] + " " + arg['string'])
                    else:
                        arg_str.append(arg['string']) 
            else:  
                arg_str = [a['string'] for a in args]
        
            # TODO: check functions with std::
            func_decl = row.f.decl().return_type().unparse().strip() + " " + row.f.name + "(" + ', '.join(arg_str) + ")"
            if row.f.name in component:
                component_func_string += func_decl + "\n" + row.f.body().unparse().replace("std::", "")
            else:
                func_prototypes.append(func_decl + ";")
                functions.append(func_decl + "\n" + row.f.body().unparse().replace("std::", ""))
            
    # for the function with pragma component, set as extern "C" 
    component_func_string = 'extern "C" {\n' + component_func_string + "}\n"

    hls_file_str = '#include "HLS/math.h"\n#include <cstdint>\n'
    hls_file_str += '#define OCL_ADDRSP_CONSTANT const __attribute__((address_space(2)))\n'
    hls_file_str += '#define OCL_ADDRSP_GLOBAL __attribute__((address_space(1)))\n'
    hls_file_str += '#define OCL_ADDRSP_PRIVATE __attribute__((address_space(0)))\n'
    hls_file_str += '#define OCL_ADDRSP_LOCAL __attribute__((address_space(3)))\n'

    # for i in global_vars:
    #     cpp_file_str += i + "\n"
    # for i in structs_and_types:
    #     cpp_file_str += i + "\n"   
    for i in func_prototypes:
        hls_file_str += i + "\n"
    for i in functions:
        hls_file_str += i + "\n"
    hls_file_str += component_func_string

    with open(path_to_hls_kernel, 'w') as f:
        f.write(hls_file_str)
    
# generate_hls_kernel('swi_project/cpp_kernel.cpp', 'swi_project/project/device/lib/library.cpp')
