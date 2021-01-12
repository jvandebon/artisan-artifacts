#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

log.level = 2

from sdt_unr import unroll_loop
from sdt_generate_opencl_kernel import generate_opencl_kernel
from udt_gen_swi_fpga_project import gen_swi_fpga_project


# TODO: check if loop has fixed bounds
def fixed_bounds(loop):
    condition = loop.cond().unparse()
    if '<' in condition:
        bound = condition.split('<')[-1].strip()[:-1]
        try:
            int(bound)
            return int(bound)
        except:
            return False
    return False


if not os.path.exists("./swi_project"):
    print("generating swi files...")
    gen_swi_fpga_project(cli())

# create ast for CPP kernel
path_to_kernel = os.getcwd() + '/swi_project/cpp_kernel.cpp'
ast = model(args='"' + path_to_kernel + '"', ws=Workspace('kernel_ws'))

# fully unroll fixed bound loops 
loops = ast.project.query('l:ForLoop')
for row in loops:
    if row.l.in_code() and fixed_bounds(row.l) != False:
        unroll_loop(ast, row.l.tag(), unroll_factor=0)
        ast.commit()

# generate OpenCL kernel 
generate_opencl_kernel(ast, 'swi_project/project/device/', None)
# ast.undo(sync=True) ## OTHERWISE #include statements are removed
# ast.commit()
subprocess.call(['cp', 'kernel_ws/default/cpp_kernel.cpp', 'swi_project/cpp_kernel.cpp'])
subprocess.call(['rm', '-rf', 'kernel_ws'])  
