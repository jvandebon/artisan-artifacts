#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')
sys.path.insert(3, '/workspace/metaprograms/udt/')

log.level = 2

from udt_gen_swi_fpga_project import gen_swi_fpga_project
from daa_check_utilisation import check_utilisation
from sdt_unr import unroll_loop
from sdt_generate_opencl_kernel import generate_opencl_kernel

# from sdt_generate_hls_kernel import generate_hls_kernel

## UDT to optimise for an unrolled single work item FPGA kernel

artisan_artifacts = '~/Documents/Artisan/artisan-artefacts/'

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

def unroll_nonfixed(ast, UF):
    # find all loops without fixed bounds
    loops = ast.project.query('l:ForLoop')
    done = []
    for row in loops:
        if row.l.in_code() and fixed_bounds(row.l) == False and row.l.tag() not in done:
            print(row.l.tag())
            # unroll_loop(ast, row.l.tag(), unroll_factor=UF)
        done.append(row.l.tag())


def unroll_fixed(ast, UF):
    # find all loops with fixed bounds
    loops = ast.project.query('l:ForLoop')
    done = []
    for row in loops:
        if row.l.in_code() and fixed_bounds(row.l) != False and row.l.tag() not in done:
            print(row.l.tag())
            unroll_loop(ast, row.l.tag(), unroll_factor=UF)
        done.append(row.l.tag())

def generate_reports():
    # make library, first stage compilation of opencl kernel 
    command = ['ssh', '-t', '172.17.0.1']
    command += ['cd', artisan_artifacts + os.getcwd().replace('workspace', '') + '/swi_project/project/', ';']
    command += ['source', '~/setup181.sh', '>', '/dev/null', '2>&1', ';']
    command += ['source', 'generate_report.sh']
    subprocess.call(command) 

def rollback(UF, ast, headers, fixed):
    print("Unrolling by %d overmapped or made no change, rolling back to %d" % (UF, int(UF/2)))
    if fixed:
        unroll_fixed(ast, int(UF/2))
    else:
        unroll_nonfixed(ast, int(UF/2))
    ast.commit()
    generate_opencl_kernel(ast, 'swi_project/project/device/', headers)


## BEGIN MAIN ##
if not os.path.exists("./swi_project"):
    gen_swi_fpga_project(cli())

# new ast for kernel
path_to_kernel = os.getcwd() + '/swi_project/cpp_kernel.cpp'
ast = model(args='"' + path_to_kernel + '"', ws=Workspace('kernel_ws'))

headers = []
header_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'header' in row.p.directive()]
for row in header_pragmas:
    directive = row.p.directive()
    headers.append(json.loads(' '.join(directive.split()[directive.split().index('header') + 1:])))

# DSE ON LOOP UNROLL FACTOR
UF = 2
overmapped = False
prev_percentages = []
while not overmapped:
    print("Trying to unroll fixed loops by factor:", UF, "...")

    # instrument code to unroll fixed bound loops
    unroll_fixed(ast, UF)
    ast.commit()
    generate_opencl_kernel(ast, 'swi_project/project/device/', headers)
    ast.undo(sync=True) ## OTHERWISE #include statements are removed
    ast.commit()

    # run first stage opencl compile to generate design reports
    generate_reports()
    # parse reports to check utilisation estimates
    try:
        utilisation = check_utilisation('./swi_project/project/bin/kernel/reports')
    except:
        rollback(UF, ast, headers, True)
        overmapped = True
        break
    
    percentages = [utilisation[u]['percentage'] for u in utilisation]
    for u in utilisation:
        print(u, utilisation[u]['percentage'], '%')
    # if no change, or if any resources are at > 100%, go back to previous unroll factor, finish 
    if max(percentages) > 90 or percentages == prev_percentages:
        rollback(UF, ast, headers, True)
        if percentages != prev_percentages:
            overmapped = True
        break
    else:  # if all resources are < 100%, increase unroll factor 
        UF = UF * 2

    prev_percentages = percentages


# if unrolling fixed loops didnt overmap, check others
UF = 2
while not overmapped:
    print("Trying to unroll NON - fixed loops by factor:", UF, "...")

    # instrument code to unroll fixed bound loops
    unroll_nonfixed(ast, UF)
    ast.commit()
    generate_opencl_kernel(ast, 'swi_project/project/device/', headers)
    ast.undo(sync=True) 
    ast.commit()

    # run first stage opencl compile to generate design reports
    generate_reports()
    # parse reports to check utilisation estimates
    try:
        utilisation = check_utilisation('./swi_project/project/bin/kernel/reports')
    except:
        rollback(UF, ast, headers, False)
        overmapped = True
        break
    
    percentages = [utilisation[u]['percentage'] for u in utilisation]
    for u in utilisation:
        print(u, utilisation[u]['percentage'], '%')
    # if no change, or if any resources are at > 100%, go back to previous unroll factor, finish 
    if max(percentages) > 90 or percentages == prev_percentages:
        rollback(UF, ast, headers, False)
        if percentages != prev_percentages:
            overmapped = True
        break
    else:  # if all resources are < 100%, increase unroll factor 
        UF = UF * 2

    prev_percentages = percentages

subprocess.call(['rm', '-rf', 'kernel_ws'])   
