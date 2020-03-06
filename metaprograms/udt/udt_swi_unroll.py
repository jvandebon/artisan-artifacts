#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

from daa_hsd import identify_hotspots
from daa_hsc import hw_synthesizable
from daa_check_utilisation import check_utilisation
from sdt_extract_hotspot import extract_hotspot
from sdt_swi_fpga import create_swi_project
from sdt_unr import unroll_loop

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

# TEMPORARY ********************************************
def fix_attributes(ast):
    project = ast.project
    func_table = project.query('g:Global => f:FnDef')
    old = []
    new = []
    for row in func_table:
        if row.f.in_code():
            s = row.f.unparse().index('(')+1
            e = row.f.unparse().index(')')
            old_args = row.f.unparse()[s:e]
            old_array = old_args.split(',')
            new_array = []
            for p in old_array:
                if '*' in p:
                    new_array.append('OCL_ADDRSP_GLOBAL ' + p)
                else:
                    new_array.append(p)
            new_args = ','.join(new_array)
            old.append(old_args)
            new.append(new_args)

    with open(os.getcwd() + '/swi_project/project/device/lib/library.cpp', 'r') as f:
        contents = f.read()
    for i in range(len(old)):
        contents = contents.replace(old[i], new[i])
    contents = contents.replace('std::', '')
    with open(os.getcwd() + '/swi_project/project/device/lib/library.cpp', 'w') as f:
        f.write(contents)
# TEMPORARY ********************************************

def generate_reports():
    # make library, first stage compilation of opencl kernel 
    command = ['ssh', '-t', '172.17.0.1']
    command += ['cd', '~/Documents/Artisan/artisan-artefacts/' + os.getcwd().replace('workspace', '') + '/swi_project/project/', ';']
    command += ['source', '~/setup.sh', '>', '/dev/null', '2>&1', ';'] # TODO: why is this necessary?
    command += ['source', 'generate_report.sh']
    subprocess.call(command) 

# meta.help('Call')
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


# new ast for library.cpp
path_to_library = os.getcwd() + '/swi_project/project/device/lib/library.cpp'
path_to_hls_include = os.getcwd() + '/swi_project/include'

ast = model(args='"' + path_to_library + '" -I/artisan/artisan/rose/repo/cpp -I' + path_to_hls_include, ws=Workspace('lib_ws'))
project = ast.project

# DSE ON LOOP UNROLL FACTOR
UF = 2
overmapped = False
prev_percentages = []
while not overmapped:
    print("Trying to unroll fixed loops by factor:", UF, "...")

    # instrument code to unroll fixed bound loops
    unroll(ast, project, UF)
    ast.commit()
    ast.export_to('./swi_project/project/device/lib/')
    
    ## TEMPORARY FIX
    fix_attributes(ast)

    # run first stage opencl compile to generate design reports
    generate_reports()
    # parse reports to check utilisation estimates
    try:
        utilisation = check_utilisation('./swi_project/project/bin/kernel/reports')
    except:
        print("Unrolling by %d overmapped, rolling back to %d" % (UF, int(UF/2)))
        ast.undo(sync=True)
        ast.commit()
        ast.export_to('./swi_project/project/device/lib/')
        fix_attributes(ast)
        break
    
    percentages = [utilisation[u]['percentage'] for u in utilisation]
    # if no change, or if any resources are at > 100%, go back to previous unroll factor, finish 
    if max(percentages) > 200 or percentages == prev_percentages:
        print("Unrolling by %d overmapped or made no change, rolling back to %d" % (UF, int(UF/2)))
        ast.undo(sync=True)
        ast.export_to('./swi_project/project/device/lib/')
        break
    else:  # if all resources are < 100%, increase unroll factor 
        UF = UF * 2

    prev_percentages = percentages

subprocess.call(['rm', '-rf', 'lib_ws'])   
