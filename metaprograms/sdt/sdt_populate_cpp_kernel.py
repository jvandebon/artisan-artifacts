#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

from utils import determine_args
from utils import get_called_funcs

import json

def func_spec_pragma(func_name, args, global_arg_names, kernels=['hotspot_func']):
    pragma = "#pragma artisan-hls spec " + func_name + " "
    for a in args:
        if (a['name'] in global_arg_names and a['is_pointer']) or (func_name in kernels and a['is_pointer']):
            if not '{' in pragma:
                pragma += " { "
            else:
                pragma += ", "
            if func_name in kernels: # and a['large']:
                pragma += '"_' + a['name'] + '" : "__global"'
            else:
                 pragma += '"' + a['name'] + '" : "__global"'
    if '{' in pragma:
        pragma += " }"
    else:
        return False
    return pragma

        
# ********************************************fm************************************
# copy specified hotspot function into C++ kernel file 
# **********************************************************************************

def populate_cpp_kernel(project, function, path_to_file, kernel_args, header, ndrange=False):

    print("populating C++ kernel ... ")

    pragmas = ["#pragma artisan-hls component hotspot_func\n"]
    func_prototypes = []
    functions = []

    # get hotspot function in input program, unparse body and determine spec pragma
    hotspot_func = project.query("g:Global => f:FnDef{%s}" % function)[0].f
    hotspot_func_args = ""
    for arg in kernel_args:
        if arg['is_pointer']:
            hotspot_func_args += arg['pointer_type']
            if arg['large'] or arg['dims'] == [] or ndrange:
                hotspot_func_args += " " + arg['name'] + ","
            else:
                hotspot_func_args += " _" + arg['name'] + ","
        else:
            hotspot_func_args += arg['type'] + " " + arg['name'] + ","
    hotspot_func_decl = hotspot_func.decl().return_type().unparse().strip() + " " + 'hotspot_func' + "(" + hotspot_func_args[:-1] + ")"
    hotspot_func_spec_pragma = func_spec_pragma('hotspot_func', kernel_args, [])
 
    # make local copies of arrays for pointers that are not large, and populate theme
    local_array_decls = ""
    populate_loops = ""
    readback_loops = ""
    global_vars = []

    for arg in kernel_args:
        if not arg['is_pointer']: # not a pointer
            continue

        if arg['large'] or arg['dims'] == [] or ndrange: # too big for local copy, not fixed size, or ndrange
            global_vars.append(arg['name'])
            hotspot_func_spec_pragma = hotspot_func_spec_pragma.replace("_"+arg['name'], arg['name'])
            continue

        if len(arg['dims']) == 1:
            local_array_decls += arg['pointer_type'].split()[0] + " " + arg['name'] + "[%d];\n" % arg['dims'][0]

            populate_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
            populate_loop += "    %s[i] = %s[i]; \n}\n" % (arg['name'], "_"+arg['name'])
            populate_loops += populate_loop

            if 'W' in arg['rw']: # only need to write back to host if going to read
                readback_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
                readback_loop += "    %s[i] = %s[i]; \n}\n" % ("_"+arg['name'], arg['name'])
                readback_loops += readback_loop
        
        #TODO: currently only support up to 2D
        if len(arg['dims']) == 2:
            
            local_array_decls += arg['pointer_type'].split()[0] + " " + arg['name'] + "[%d][%d];\n" % (arg['dims'][0], arg['dims'][1])
            
            populate_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
            populate_loop += "    for (int j=0;j<%s;j++) {\n" % str(arg['dims'][1])
            populate_loop += "        %s[i][j] = %s[i*%s+j];\n    }\n}\n" % (arg['name'], "_"+arg['name'], str(arg['dims'][1]))
            populate_loops += populate_loop
            
            if 'W' in arg['rw']:
                readback_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
                readback_loop += "    for (int j=0;j<%s;j++) {\n" % str(arg['dims'][1])
                readback_loop += "        %s[i*%s+j] = %s[i][j];\n    }\n}\n" % ("_"+arg['name'], str(arg['dims'][1]), arg['name'])
                readback_loops += readback_loop
        
        
    pragmas.append(hotspot_func_spec_pragma)
    hotspot_func.body().instrument(pos='begin', code=local_array_decls+populate_loops)
    hotspot_func.body().instrument(pos='end', code=readback_loops)
    hotspot_func_string = hotspot_func_decl + "\n" + hotspot_func.body().unparse()

    all_global_vars = {}
    all_global_vars[function] = global_vars

    # recursively find all functions called in hotspot
    funcs_to_copy_over = []
    get_called_funcs(project, hotspot_func, funcs_to_copy_over)
    for func_ in funcs_to_copy_over:
        func = func_[0]
        call = func_[3]
        parent = func_[2]
        if not func.name in all_global_vars:
            all_global_vars[func.name] = []
        args = determine_args(func.decl().params(), {}, {})
        call_params = call[call.index("(")+1:call.rfind(")")].split(",")
        global_arg_names = []
        for p in call_params:
            for v in all_global_vars[parent]:
                if v in p:
                    if p.index(v) == 0 or not (p[p.index(v)-1].isalpha() or p[p.index(v)-1].isnumeric()):
                        if p.index(v)+len(v) >= len(p) or not (p[p.index(v)+len(v)].isalpha() or p[p.index(v)+len(v)].isnumeric()):
                            global_arg_names.append(args[call_params.index(p)]['name'])
                            all_global_vars[func.name].append(args[call_params.index(p)]['name'])
        func_args = ""
        for arg in args:
            if arg['is_pointer']:
                if len(arg['dims']) > 0:
                    func_args += arg['pointer_type'].split()[0] + " " + arg['name']
                    for dim in arg['dims']:
                        func_args += "["+str(dim)+"]"
                    func_args += ","
                else:
                    func_args += arg['pointer_type'] + " " + arg['name'] + ","
            else:
                func_args += arg['type'] + " " + arg['name'] + ","
                
        func_decl = func.decl().return_type().unparse().strip() + " " + func.name + "(" + func_args[:-1].replace("const", "") + ")"
        if len(global_arg_names) != 0:
            pragmas.append(func_spec_pragma(func.name, args, global_arg_names))
        func_prototypes.append(func_decl + ";")
        
        functions.append(func_decl + "\n" + func.body().unparse())

    cpp_file_str = "#include <cmath>\n#include <cstdint>\n"
    for h in header:
        if 'host_only' in h:
            continue
        cpp_file_str += "#include \"" + h['file'] + "\"\n"
        cpp_file_str += "#pragma artisan-hls header " + json.dumps(h) + "\n"

    
    type_pragmas = [row for row in project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'types' in row.p.directive()]
    for row in type_pragmas:
        pragmas.append(row.p.unparse().strip())

    for i in pragmas:
        cpp_file_str += i + "\n"  
    for i in func_prototypes:
        cpp_file_str += i + "\n"
    for i in functions:
        cpp_file_str += i + "\n"
    cpp_file_str += hotspot_func_string

    with open(path_to_file, 'w') as f:
        f.write(cpp_file_str)


def populate_cpp_kernel_multi(project, kernels, path_to_file, kernel_args, header):

    print("populating C++ kernel ... ")

    pragmas = []
    func_prototypes = []
    functions = []
    pragmas = []
    kernel_strings = []

    for function in kernels:
        global_vars = []
        pragmas.append("#pragma artisan-hls component %s\n" % function)

        # get hotspot function in input program, unparse body and determine spec pragma
        hotspot_func = project.query("f:FnDef{%s}" % function)[0].f
        hotspot_func_args = ""
        for arg in kernel_args[function]:
            if arg['is_pointer']:
                hotspot_func_args += arg['pointer_type']
                if len(arg['dims'])==2:
                    hotspot_func_args += " _" + arg['name'] + ","
                else:
                    hotspot_func_args += " " + arg['name'] + ","
            else:
                hotspot_func_args += arg['type'] + " " + arg['name'] + ","

        hotspot_func_decl = hotspot_func.decl().return_type().unparse().strip() + " " + function + "(" + hotspot_func_args[:-1] + ")"
        hotspot_func_spec_pragma = func_spec_pragma(function, kernel_args[function], [], kernels=kernels)   

        global_vars = []        
        local_array_decls = ""
        populate_loops = ""
        readback_loops = ""
        
        for arg in kernel_args[function]:
            if not arg['is_pointer']: # not a pointer
                continue

            if arg['large'] or len(arg['dims']) != 2: # too big for local copy, not fixed size, or ndrange
                global_vars.append(arg['name'])
                hotspot_func_spec_pragma = hotspot_func_spec_pragma.replace("_"+arg['name'], arg['name'])
                continue

            #TODO: currently only support up to 2D
            if len(arg['dims']) == 2:
                
                local_array_decls += arg['pointer_type'].split()[0] + " " + arg['name'] + "[%d][%d];\n" % (arg['dims'][0], arg['dims'][1])
                
                populate_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
                populate_loop += "    for (int j=0;j<%s;j++) {\n" % str(arg['dims'][1])
                populate_loop += "        %s[i][j] = %s[i*%s+j];\n    }\n}\n" % (arg['name'], "_"+arg['name'], str(arg['dims'][1]))
                populate_loops += populate_loop
                
                if 'W' in arg['rw']:
                    readback_loop = "for (int i=0;i<%s;i++) {\n" % str(arg['dims'][0])
                    readback_loop += "    for (int j=0;j<%s;j++) {\n" % str(arg['dims'][1])
                    readback_loop += "        %s[i*%s+j] = %s[i][j];\n    }\n}\n" % ("_"+arg['name'], str(arg['dims'][1]), arg['name'])
                    readback_loops += readback_loop
            
        
        pragmas.append(hotspot_func_spec_pragma)
        hotspot_func.body().instrument(pos='begin', code=local_array_decls+populate_loops)
        hotspot_func.body().instrument(pos='end', code=readback_loops)
        hotspot_func_string = hotspot_func_decl + "\n" + hotspot_func.body().unparse()

        all_global_vars = {}
        all_global_vars[function] = global_vars

        # recursively find all functions called in hotspot
        funcs_to_copy_over = []
        get_called_funcs(project, hotspot_func, funcs_to_copy_over)
        for func_ in funcs_to_copy_over:
            func = func_[0]
            call = func_[3]
            parent = func_[2]
            if not func.name in all_global_vars:
                all_global_vars[func.name] = []
            args = determine_args(func.decl().params(), {}, {})
            call_params = call[call.index("(")+1:call.rfind(")")].split(",")
            global_arg_names = []
            for p in call_params:
                for v in all_global_vars[parent]:
                    if v in p:
                        if p.index(v) == 0 or not (p[p.index(v)-1].isalpha() or p[p.index(v)-1].isnumeric()):
                            if p.index(v)+len(v) >= len(p) or not (p[p.index(v)+len(v)].isalpha() or p[p.index(v)+len(v)].isnumeric()):
                                global_arg_names.append(args[call_params.index(p)]['name'])
                                all_global_vars[func.name].append(args[call_params.index(p)]['name'])
            func_args = ""
            for arg in args:
                if arg['is_pointer']:
                    if len(arg['dims']) > 0:
                        func_args += arg['pointer_type'].split()[0] + " " + arg['name']
                        for dim in arg['dims']:
                            func_args += "["+str(dim)+"]"
                        func_args += ","
                    else:
                        func_args += arg['pointer_type'] + " " + arg['name'] + ","
                else:
                    func_args += arg['type'] + " " + arg['name'] + ","
                
            func_decl = func.decl().return_type().unparse().strip() + " " + func.name + "(" + func_args[:-1].replace("const", "") + ")"
            if len(global_arg_names) != 0:
                x = func_spec_pragma(func.name, args, global_arg_names)
                if x != False:
                    pragmas.append(x)
            func_prototypes.append(func_decl + ";")
        
            functions.append(func_decl + "\n" + func.body().unparse())

        kernel_strings.append(hotspot_func_string)

    cpp_file_str = "#include <cmath>\n#include <cstdint>\n"
    for h in header:
        if 'host_only' in h:
            continue
        cpp_file_str += "#include \"" + h['file'] + "\"\n"
        cpp_file_str += "#pragma artisan-hls header " + json.dumps(h) + "\n"

    
    type_pragmas = [row for row in project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'types' in row.p.directive()]
    for row in type_pragmas:
        pragmas.append(row.p.unparse().strip())

    for i in pragmas:
        cpp_file_str += i + "\n"  
    for i in func_prototypes:
        cpp_file_str += i + "\n"
    for i in functions:
        cpp_file_str += i + "\n"
    for i in kernel_strings:
        cpp_file_str += i + "\n"

    with open(path_to_file, 'w') as f:
        f.write(cpp_file_str)