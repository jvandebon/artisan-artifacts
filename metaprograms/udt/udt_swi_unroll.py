#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

from daa_hsd import identify_hotspots
from sdt_extract_hotspot import extract_hotspot
from sdt_swi_fpga import create_swi_project



## UDT to optimise for an unrolled single work item FPGA kernel

daa_ast = model(args=cli(), ws=Workspace('daa'))

# identify hotspot loop for acceleration
if os.path.exists("./hotspot-result"):
    with open('./hotspot-result/hotspots.json', 'r') as json_file:
        hotspots= json.load(json_file)
else:
    hotspots = identify_hotspots(daa_ast, 50.0)

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

# TODO: check for unsynthesizable constructts

# extract hotspot
ast = model(args=cli(), ws=Workspace('project'))
# TODO: assumes only outermost loops so can get function name easy, double check this
extract_hotspot(ast, hotspot_loop_tag, '_'.join(hotspot_loop_tag.split('_')[:-2]))

# create swi project
create_swi_project(ast, "/workspace/metaprograms/templates/", "project/default" )


# TODO: SDTs on hls library function for optimisation 

