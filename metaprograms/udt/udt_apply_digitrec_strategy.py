#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

log.level = 2

from sdt_unr import unroll_loop
from sdt_inline_function import inline_function
from sdt_generate_opencl_kernel import generate_opencl_kernel
from udt_gen_ndrange_fpga_project import gen_ndrange_fpga_project
from sdt_modify_ndrange_params import modify_ndrange_params
from sdt_unr import unroll_loop

if not os.path.exists("./ndrange_project"):
    print("generating ndrange files...")
    gen_ndrange_fpga_project(cli())

# meta.help('ForLoop')
# create ast for CPP kernel
path_to_kernel = os.getcwd() + '/ndrange_project/cpp_kernel.cpp'
ast = model(args='"' + path_to_kernel + '"', ws=Workspace('kernel_ws'))

headers = []
header_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'header' in row.p.directive()]
for row in header_pragmas:
    directive = row.p.directive()
    headers.append(json.loads(' '.join(directive.split()[directive.split().index('header') + 1:])))


# inline all functions 
inline_function(ast, 'popcount_', 'update_knn')
inline_function(ast, 'update_knn', 'hotspot_func')
inline_function(ast, 'knn_vote', 'hotspot_func')

# unroll loop partially 
unroll_loop(ast, 'hotspot_func_for_a_b', unroll_factor=16)

generate_opencl_kernel(ast, 'ndrange_project/project/device/', headers, ndrange=True)
subprocess.call(['cp', 'kernel_ws/default/cpp_kernel.cpp', 'ndrange_project/cpp_kernel.cpp'])
subprocess.call(['rm', '-rf', 'kernel_ws'])  

# set ndrange params
modify_ndrange_params(os.getcwd() + '/ndrange_project/project/device/kernel.cl', 16, 16)

