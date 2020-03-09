#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

log.level = 2

from daa_hsd import identify_hotspots
from daa_hsc import hw_synthesizable
from daa_check_utilisation import check_utilisation
from sdt_extract_hotspot import extract_hotspot
from sdt_swi_fpga import create_swi_project
from sdt_unr import unroll_loop
from sdt_generate_hls_kernel import generate_hls_kernel

## UDT to optimise for an unrolled single work item FPGA kernel

# TODO: how to check if loop has fixed bounds?
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

def unroll(ast, project, UF):
# find all loops with fixed bounds
    loops = project.query('l:ForLoop')
    for row in loops:
        # TODO: check if fixed bounds
        if fixed_bounds(row.l) != False:
            unroll_loop(ast, row.l.tag(), unroll_factor=UF)

def generate_reports():
    # make library, first stage compilation of opencl kernel 
    command = ['ssh', '-t', '172.17.0.1']
    command += ['cd', '~/Documents/Artisan/artisan-artefacts/' + os.getcwd().replace('workspace', '') + '/swi_project/project/', ';']
    command += ['source', '~/setup.sh', '>', '/dev/null', '2>&1', ';'] # TODO: why is this necessary?
    command += ['source', 'generate_report.sh']
    subprocess.call(command) 

def rollback(UF, ast):
    print("Unrolling by %d overmapped or made no change, rolling back to %d" % (UF, int(UF/2)))
    ast.undo(sync=True)
    ast.commit()
    generate_hls_kernel(ast.project, 'swi_project/project/device/lib/library.cpp')

if not os.path.exists("./swi_project"):

    # identify hotspot loop for acceleration
    if os.path.exists("./hotspot-result"):
        with open('./hotspot-result/hotspots.json', 'r') as json_file:
            hotspots=json.load(json_file)
    else:
        daa_ast = model(args=cli(), ws=Workspace('daa'))
        hotspots = identify_hotspots(daa_ast, 50.0)
        subprocess.call(['rm', '-rf', 'daa'])

    hotspot_loops = [tag for tag in hotspots if tag != 'main']
    if len(hotspot_loops) > 1:
        print("More than one hotspot loop identified.")
        # TODO: get user input / further analysis
        exit(0)
    elif len(hotspot_loops) == 0:
        print("No hotspot identified for acceleration.")
        # TODO: get user input / further analysis
    else:
        hotspot_loop_tag = hotspot_loops[0]

    # extract hotspot
    ast = model(args=cli(), ws=Workspace('swi_ws'))

    # assumes only outermost loops so can get function name easy, double check this
    extract_hotspot(ast, hotspot_loop_tag, '_'.join(hotspot_loop_tag.split('_')[:-2]))

    # check for unsynthesizable constructs
    if not hw_synthesizable(ast, 'hotspot'):
        print("The identified hotspot contains constructs that are not HW syntheszable.")
        exit(0)

    # create swi project
    create_swi_project(ast, "/workspace/metaprograms/templates/", "swi_ws/default")
    subprocess.call(['rm', '-rf', 'swi_ws'])

if os.path.exists("./swi_project"):
    # new ast for kernel
    path_to_kernel = os.getcwd() + '/swi_project/cpp_kernel.cpp'
    ## BUG: signal clash or segmentation fault happens here. 
    ast = model(args='"' + path_to_kernel + '"', ws=Workspace('kernel_ws'))

    # DSE ON LOOP UNROLL FACTOR
    UF = 2
    overmapped = False
    prev_percentages = []
    while not overmapped:
        print("Trying to unroll fixed loops by factor:", UF, "...")

        # instrument code to unroll fixed bound loops
        unroll(ast, ast.project, UF)
        ast.commit()

        generate_hls_kernel(ast.project, 'swi_project/project/device/lib/library.cpp')

        # run first stage opencl compile to generate design reports
        generate_reports()
        # parse reports to check utilisation estimates
        try:
            utilisation = check_utilisation('./swi_project/project/bin/kernel/reports')
        except:
            rollback(UF, ast)
            break
        
        percentages = [utilisation[u]['percentage'] for u in utilisation]
        # if no change, or if any resources are at > 100%, go back to previous unroll factor, finish 
        if max(percentages) > 90 or percentages == prev_percentages:
            rollback(UF, ast)
            break
        else:  # if all resources are < 100%, increase unroll factor 
            UF = UF * 2

        prev_percentages = percentages

    subprocess.call(['rm', '-rf', 'kernel_ws'])   
