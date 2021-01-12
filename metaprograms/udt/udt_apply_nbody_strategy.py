#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time
log.level = 2

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

from sdt_remove_mem_dep_with_register import remove_dep_with_register
from sdt_generate_opencl_kernel import generate_opencl_kernel
from udt_gen_ndrange_fpga_project import gen_ndrange_fpga_project
from sdt_modify_ndrange_params import modify_ndrange_params

if not os.path.exists("./ndrange_project"):
    print("generating ndrange files...")
    gen_ndrange_fpga_project(cli())

path_to_kernel = os.getcwd() + '/ndrange_project/cpp_kernel.cpp'
ast = model(args='"' + path_to_kernel + '"', ws=Workspace('kernel_ws'))
project = ast.project

headers = []
header_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'header' in row.p.directive()]
for row in header_pragmas:
    directive = row.p.directive()
    headers.append(json.loads(' '.join(directive.split()[directive.split().index('header') + 1:])))


# find loop with #pragma artisan-hls mem_dep
dep_pragmas = [row for row in project.query("l:ForLoop => p:Pragma") if 'artisan-hls' in row.p.directive() and 'mem_dep' in row.p.directive()]
for row in dep_pragmas:
    loop = row.l 
    pragma = row.p
    deps = json.loads(' '.join(pragma.directive().split()[pragma.directive().split().index('mem_dep') + 1:]))
    if loop.tag() in deps:
        # call remove_dep_with_register
        remove_dep_with_register(loop, deps[loop.tag()])
        ast.commit()


# generate OpenCL kernel 
generate_opencl_kernel(ast, 'ndrange_project/project/device/', headers, ndrange=True)

# update CPP kernel
subprocess.call(['cp', 'kernel_ws/default/cpp_kernel.cpp', 'ndrange_project/cpp_kernel.cpp'])
subprocess.call(['rm', '-rf', 'kernel_ws'])  

# set ndrange params
modify_ndrange_params(os.getcwd() + '/ndrange_project/project/device/kernel.cl', 16, 16)