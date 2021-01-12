#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import os, subprocess, json

from sdt_populate_cpp_kernel import populate_cpp_kernel
from sdt_populate_opencl_host import populate_opencl_host
from sdt_generate_opencl_kernel import generate_opencl_kernel
from utils import *

def get_global_work_size(hotspot_func):
    loop = hotspot_func.query('l:ForLoop')[0]
    if loop.l.is_outermost():
        size = loop.l.cond().unparse().split('<')[-1].replace(';', '')
        return size.strip()

def create_ndrange_project(ast, template_path, source_path, hotspot, header):

    # make copy of template project directory
    if not os.path.exists("./ndrange_project"):
        subprocess.call(['cp', '-r', template_path+'ndrange_project', '.'])

    # figure out hotspot function call params
    project = ast.project
    hotspot_func = project.query("g:Global => f:FnDef{%s}" % hotspot)[0].f 
    params = hotspot_func.decl().params()

    # pragmas may specify if variable are R/W/RW
    vars_rw = check_vars_rw(hotspot_func)

    # pragmas may specify pointer sizes
    pointer_sizes = check_pointer_sizes(hotspot_func)

    # determine arg names, types, sizes (if static)
    args = determine_args(params, vars_rw, pointer_sizes)

    # if ndrange, determine global work size
    size = get_global_work_size(hotspot_func)

    # generate opencl host template file
    populate_opencl_host(ast, source_path, 'ndrange_project/project/host/src/', args, header, hotspot, ndrange=True, gWorkSize=size)
  
    # gennerate c++ version of kernel (with pragmas to guide opencl kernel generation)
    populate_cpp_kernel(project, hotspot, 'ndrange_project/cpp_kernel.cpp', args, header,ndrange=True)

    # copy over header files if needed (pragmas specify)
    copy_header_files(header, 'ndrange_project/')

    # generate OpenCL kernel from c++ kerenel
    path_to_cpp_kernel = os.getcwd() + '/ndrange_project/cpp_kernel.cpp'
    ast = model(args='"' + path_to_cpp_kernel + '"', ws=Workspace('ws'))
    generate_opencl_kernel(ast, os.getcwd() + '/ndrange_project/project/device/', header, ndrange=True)
    subprocess.call(['rm', '-rf', 'ws'])

    