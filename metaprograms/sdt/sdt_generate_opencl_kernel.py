#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import json

from utils import determine_args

def generate_opencl_kernel(ast, path_to_kernel, header, ndrange=False):
    print("generating OpenCL kernel ... ")
    
    ocl_file_str = ""

    # find all pragma statements
    project = ast.project
    pragmas = project.query('g:Global => p:Pragma')
    component = ""
    specs = {}
    for row in [r for r in pragmas if r.p.in_code() and 'artisan-hls' in r.p.directive()]:
            directive = row.p.directive()
            if 'artisan-hls component' in directive:
                idx = directive.split().index('component') + 1
                component = directive.split()[idx]
            elif 'artisan-hls spec' in directive:
                func_idx = directive.split().index('spec') + 1 
                func = directive.split()[func_idx]
                spec = json.loads(' '.join(directive.split()[func_idx+1:]))
                specs[func] = spec
            elif 'artisan-hls types' in directive:
                idx = directive.split().index('types') + 1
                types = json.loads(' '.join(directive.split()[idx:]))
                for t in types:
                    ocl_file_str += "#define "+t+" "+types[t]+"\n"
            elif 'artisan-hls' in directive:
                row.p.instrument(pos='replace', code='')
           

    if component ==  "":
        print("No kernel function specified.")
        exit(0)

    func_prototypes = []
    functions = []
    component_func_string = "kernel "
    
    # find all functions
    function_table = project.query("g:Global => f:FnDef")
    for row in function_table:
        if row.f.in_code():
            args = determine_args(row.f.decl().params(), {}, {})
            # if so, set up args as specified, unparse, set up prototype
            arg_str = []
            for arg in args:

                if row.f.name == 'hotspot_func' and row.f.name in specs and arg['name'] in specs[row.f.name]:  
                    string = arg['pointer_type'].replace("*", "* restrict") + " "
                    if arg['large'] or len(arg['dims']) == 0:
                        string += arg['name']
                    else:
                        string += "_"+arg['name']                        
                    arg_str.append(specs[row.f.name][arg['name']] + " " + string)

                elif row.f.name in specs and arg['name'] in specs[row.f.name]:
                    string = arg['pointer_type'] + " " + arg['name']
                    arg_str.append(specs[row.f.name][arg['name']] + " " + string)
                
                else:
                    if arg['is_pointer']:
                        if len(arg['dims']) > 0:
                            string = arg['pointer_type'].split()[0] + " " + arg['name']
                            for dim in arg['dims']:
                                string += "["+str(dim)+"]"
                        else:
                            string = arg['pointer_type'] + " " + arg['name']
                        arg_str.append(string)
                    else:
                        arg_str.append(arg['type'] + " " + arg['name']) 
        
            func_decl = row.f.decl().return_type().unparse().strip() + " " + row.f.name + "(" + ', '.join(arg_str) + ")"
            if row.f.name in component:
                
                # IF NDRANGE KERNEL, REMOVE OUTER LOOP, CALL GET_GLOBAL_ID(), add SIMD/WG attributes
                if ndrange:
                    loops = row.f.body().query('l:ForLoop')
                    for loop in loops:
                        if loop.l.is_outermost():
                            init_stmt = loop.l.init()[0].unparse()
                            id_var = init_stmt[:init_stmt.index('=')].split()[-1]
                            tmp_body = loop.l.body().unparse()
                            new_body = "    int " + id_var + " = get_global_id(0);\n" + tmp_body[tmp_body.index('{')+1:tmp_body.rfind('}')]
                            loop.l.instrument(pos='replace', code=new_body)
                        component_func_string = "#define WG 4\n#define SIMD 4\n__attribute__((reqd_work_group_size(WG, 1, 1)))\n__attribute__((num_simd_work_items(SIMD)))\nkernel " 
                
                body = row.f.body().unparse() 
                component_func_string += func_decl.replace(component, 'hotspot_kernel') + "\n" + body 
            else:
                func_prototypes.append(func_decl+ ";")
                body = row.f.body().unparse()
                functions.append(func_decl + "\n" + body)

    if header != None:
        for h in header:
            if 'host_only' not in h:
                ocl_file_str += "#include \"" + h['file'] + "\"\n"
                # replace const definitions in header with __constant
                with open(path_to_kernel+h['file'], 'r') as f:
                    contents = f.read()
                    contents = contents.replace('const ', '__constant ')
                    contents = contents.replace('static', '')
        
                with open(path_to_kernel+h['file'], 'w') as f:
                    f.write(contents)

    for i in func_prototypes:
        ocl_file_str += i + "\n"
    for i in functions:
        ocl_file_str += i + "\n"
    ocl_file_str += component_func_string

    # BUG: issues with :: namespace, static
    ocl_file_str = ocl_file_str.replace("std::", "")
    ocl_file_str = ocl_file_str.replace("::", "")
    ocl_file_str = ocl_file_str.replace("static", "")

    with open(path_to_kernel+"kernel.cl", 'w') as f:
        f.write(ocl_file_str)
