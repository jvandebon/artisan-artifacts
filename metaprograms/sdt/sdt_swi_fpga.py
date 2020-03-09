#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import os, subprocess, json

from sdt_generate_hls_kernel import generate_hls_kernel
from sdt_populate_cpp_kernel import populate_cpp_kernel
from sdt_populate_opencl_kernel import populate_opencl_kernel
from sdt_populate_opencl_host import populate_opencl_host

def create_swi_project(ast, template_path, source_path):

    project = ast.project
    # make copy of template project directory
    if not os.path.exists("./swi_project"):
        print("copying over template files ... ")
        subprocess.call(['cp', '-r', template_path+'swi_project', '.'])

    # figure out all hotspot function arguments 
    hotspot_func = project.query("g:Global => f:FnDef{hotspot}")[0].f 
    params = hotspot_func.decl().params()

    var_pragmas = [row for row in hotspot_func.query("p:Pragma") if 'artisan-hls' in row.p.directive() and 'vars' in row.p.directive()]
    if len(var_pragmas) > 0:
        directive = var_pragmas[0].p.directive()
        vars_rw = json.loads(' '.join(directive.split()[directive.split().index('vars') + 1:]))
    else:
        vars_rw = {}

    # determine args, types, sizes if static 
    args = []
    for p in params:
        arg = {}
        arg['name'] = p.name
        arg['type'] = p.type().unparse().strip()
        arg['string'] = ""
        # TODO: can't handle 2d array size 
        if "[" in arg['type'] and "]" in arg['type']: 
            print(arg['type'], arg['name'])
            print(arg['type'].split('['))
            arg['string'] = arg['type'].split('[')[0] + " * " + arg['name'] 
            if len(arg['type'].split('[')) > 2:
                size = 1
                for i in range(1, len(arg['type'].split('['))):
                    size *= int(arg['type'].split('[')[i][:-1])
                arg['size'] = str(size) + "*sizeof(" + arg['type'].split('[')[0] + ")"
            else:
                arg['size'] = arg['type'].split('[')[-1][:-1] + "*sizeof(" + arg['type'].split('[')[0] + ")"
            arg['type'] = arg['type'].split('[')[0] + " * "
        else:
            arg['size'] = ""
            arg['string'] = arg['type'] + " " + arg['name']
        if arg['name'] in vars_rw:
            arg['rw'] = vars_rw[arg['name']]
        else:
            arg['rw'] = 'RW' 
        args.append(arg)
    
    populate_cpp_kernel(project, 'hotspot', 'swi_project/cpp_kernel.cpp', args)
    populate_opencl_kernel('swi_project/project/device/kernel.cl', args)
    populate_opencl_host(ast, source_path, 'swi_project/project/host/src/', args)
    path_to_kernel = os.getcwd() + '/swi_project/cpp_kernel.cpp'
    ast = model(args='"' + path_to_kernel + '"', ws=Workspace('ws'))

    kernel_project = generate_hls_kernel(ast.project, 'swi_project/project/device/lib/library.cpp')
    
    subprocess.call(['rm', '-rf', 'ws'])

# ast = model(args=cli(), ws=Workspace('temp'))
# create_swi_project(ast, "/workspace/metaprograms/templates/", "temp/default")