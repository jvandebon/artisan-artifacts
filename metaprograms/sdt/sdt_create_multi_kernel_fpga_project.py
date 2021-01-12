#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import os, subprocess, json
from utils import *

from sdt_populate_cpp_kernel import populate_cpp_kernel_multi

def create_multi_kernel_project(ast, kernels, template_path, header): # source_path

    # make copy of template project directory
    if not os.path.exists("./fpga_project"):
        subprocess.call(['cp', '-r', template_path+'fpga_project', '.'])

    project = ast.project

    kernel_args = {}

    for func_tag in kernels:
        print("\n ***** %s *****" % func_tag)

        func = project.query("f:FnDef{%s}" % func_tag)[0].f 
        params = func.decl().params()

        # pragmas may specify if variable are R/W/RW
        vars_rw = check_vars_rw(func)

        # pragmas may specify pointer sizes
        pointer_sizes = check_pointer_sizes(func)

        # determine arg names, types, sizes (if static)
        kernel_args[func_tag] = determine_args(params, vars_rw, pointer_sizes)  
        for a in kernel_args[func_tag]:
            print(a)  
    
    populate_cpp_kernel_multi(project, kernels, "fpga_project/cpp_kernel.cpp", kernel_args, header)
    copy_header_files(header, 'fpga_project/')
    path_to_cpp_kernel = os.getcwd() + '/fpga_project/cpp_kernel.cpp'
    ast = model(args='"' + path_to_cpp_kernel + '"', ws=Workspace('ws'))
    subprocess.call(['rm', '-rf', 'ws'])

ast = model(args=cli(), ws=Workspace('tmp'))
header = []
header_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'header' in row.p.directive()]
for row in header_pragmas:
    directive = row.p.directive()
    header.append(json.loads(' '.join(directive.split()[directive.split().index('header') + 1:])))

create_multi_kernel_project(ast, ['projection','rasterization1','rasterization2','zculling','coloringFB'], "/workspace/metaprograms/templates/",header)
