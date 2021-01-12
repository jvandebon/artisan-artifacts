#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import os, subprocess, json

from sdt_populate_cpp_kernel import populate_cpp_kernel
from sdt_populate_opencl_host import populate_opencl_host
from sdt_generate_opencl_kernel import generate_opencl_kernel
from utils import *


def create_swi_project(ast, template_path, source_path, hotspot, header):

    # make copy of template project directory
    if not os.path.exists("./swi_project"):
        subprocess.call(['cp', '-r', template_path+'swi_project', '.'])

    # get hotspot function call params (kernel args)
    project = ast.project
    hotspot_func = project.query("g:Global => f:FnDef{%s}" % hotspot)[0].f 
    params = hotspot_func.decl().params()

    # analysis: variables R/W/RW, pointer sizes
    vars_rw = check_vars_rw(hotspot_func)
    pointer_sizes = check_pointer_sizes(hotspot_func)

    # determine arg names, types, sizes (if static)
    args = determine_args(params, vars_rw, pointer_sizes)

    # generate opencl host 
    populate_opencl_host(ast, source_path, 'swi_project/project/host/src/', args, header, hotspot)
    
    # gennerate c++ version of kernel (with pragmas to guide opencl kernel generation)
    populate_cpp_kernel(project, hotspot, 'swi_project/cpp_kernel.cpp', args, header)

    # copy over header files if needed (pragmas specify)
    copy_header_files(header, 'swi_project/')

    # generate OpenCL kernel from c++ kernel
    path_to_cpp_kernel = os.getcwd() + '/swi_project/cpp_kernel.cpp'
    ast = model(args='"' + path_to_cpp_kernel + '"', ws=Workspace('ws'))
    generate_opencl_kernel(ast, os.getcwd() + '/swi_project/project/device/', header)
    subprocess.call(['rm', '-rf', 'ws'])

    return 