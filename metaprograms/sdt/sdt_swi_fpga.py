#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess 
import os

from sdt_populate_hls_library import populate_hls_library
from sdt_populate_opencl_kernel import populate_opencl_kernel
from sdt_populate_opencl_host import populate_opencl_host
log.level = 2


# def __main__():

# inputs: template path, c++ source
template_path = "/workspace/metaprograms/templates/"
source_path = "./optimisation_project/spam_filter.cpp"

def create_swi_project(ast, template_path, source_path):
    # ast = model(args=cli(), ws=Workspace('project'))
    project = ast.project
    
    # make copy of template project directory
    if not os.path.exists("./swi_project"):
        print("copying over template files ... ")
        subprocess.call(['cp', '-r', template_path+'swi_project', '.'])

    # figure out all hotspot function arguments 

    hotspot_func = project.query("g:Global => f:FnDef", where="f.name == 'hotspot'")[0].f 
    params = hotspot_func.decl().params()

    # determine args, types, sizes if static 
    args = []
    for p in params:
        arg = {}
        arg['name'] = p.name
        arg['type'] = p.type().unparse().strip()
        arg['string'] = ""
        if "[" in arg['type'].split()[-1] and "]" in arg['type'].split()[-1]: 
        #    arg['string'] = ' '.join(arg['type'].split()[:-1]) + " " + arg['name'] + arg['type'].split()[-1]
            arg['string'] = ' '.join(arg['type'].split()[:-1]) + " * " + arg['name'] 
            arg['size'] = arg['type'].split()[-1][1:-1] + "*sizeof(" + ' '.join(arg['type'].split()[:-1]) + ")"
            arg['type'] = ' '.join(arg['type'].split()[:-1]) + " * "
        else:
            arg['size'] = ""
            arg['string'] = arg['type'] + " " + arg['name']
        arg['rw'] = 'CL_MEM_READ_WRITE' # TODO
        args.append(arg)

    populate_hls_library(project, 'swi_project/project/device/lib/library.cpp', args)
    populate_opencl_kernel('swi_project/project/device/kernel.cl', args)
    populate_opencl_host(ast, source_path, 'swi_project/project/host/src/', args)
