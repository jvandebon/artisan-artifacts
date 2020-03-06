#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import os, subprocess 

from sdt_populate_hls_library import populate_hls_library
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
        arg['rw'] = 'CL_MEM_READ_WRITE' # TODO
        args.append(arg)

    populate_hls_library(project, 'swi_project/project/device/lib/library.cpp', args)
    populate_opencl_kernel('swi_project/project/device/kernel.cl', args)
    populate_opencl_host(ast, source_path, 'swi_project/project/host/src/', args)
