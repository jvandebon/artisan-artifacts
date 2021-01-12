#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess, time

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

log.level = 2

from daa_hsd import hotspot_detection
from daa_hsc import hw_synthesizable
from sdt_create_multi_kernel_fpga_project import create_multi_kernel_project

## BEGIN MAIN ##
def gen_multi_kernel_fpga_project(cli):
    if not os.path.exists("./multikernel_project"):

        # identify hotspot loop for acceleration
        if os.path.exists("./hotspot-result"):
            with open('./hotspot-result/hotspots.json', 'r') as json_file:
                hotspots = json.load(json_file)
        else:
            daa_ast = model(args=cli, ws=Workspace('daa'))
            hotspots = hotspot_detection(daa_ast, 50.0)
            subprocess.call(['rm', '-rf', 'daa'])
        
        hotspot_tags = [tag for tag in hotspots if tag != 'main']

        if len(hotspot_tags) > 1:
            print("More than one hotspot identified, selecting largest...")
            percentages = [hotspots[h]['percentage'] for h in hotspots if h != 'main']
            max_percent = max(percentages)
            hotspot_tags = [hotspot_tags[percentages.index(max_percent)]]
            # TODO: get user input / further analysis
        elif len(hotspots) == 0:
            print("No hotspot identified for acceleration.")
            # TODO: get user input / further analysis
            exit(0)

        if '_for_' in hotspot_tags[0] or '_while_' in hotspot_tags[0]:
            hotspot_loop_tag = hotspot_tags[0]
            hotspot_func = False
        else:
            hotspot_func = hotspot_tags[0]

        # extract hotspot loop (unless hotspot is already a function)
        ast = model(args=cli, ws=Workspace('multikernel_ws'))
        if hotspot_func == False:
            # assumes only outermost loops 
            extract_hotspot_loop_to_function(ast, hotspot_loop_tag, '_'.join(hotspot_loop_tag.split('_')[:-2]))
            hotspot_func = "hotspot"

        # check for unsynthesizable constructs
        if not hw_synthesizable(ast, hotspot_func):
            print("The identified hotspot contains constructs that are not HW syntheszable.")
            exit(0)

        # create swi project
        header = []
        header_pragmas = [row for row in ast.project.query("g:Global => p:Pragma") if 'artisan-hls' in row.p.directive() and 'header' in row.p.directive()]
        for row in header_pragmas:
            directive = row.p.directive()
            header.append(json.loads(' '.join(directive.split()[directive.split().index('header') + 1:])))

        create_multi_kernel_project(ast, "/workspace/metaprograms/templates/", "multikernel_ws/default", hotspot_func, header)
        subprocess.call(['rm', '-rf', 'multikernel_ws'])

gen_multi_kernel_fpga_project(cli())