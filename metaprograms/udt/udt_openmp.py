#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

from daa_hsd import identify_hotspots
from sdt_extract_hotspot import extract_hotspot
from sdt_openmp import multithread_loop

## UDT to optimise by multithreading with OpenMP

# identify hotspot loop for acceleration
if os.path.exists("./hotspot-result"):
    with open('./hotspot-result/hotspots.json', 'r') as json_file:
        hotspots= json.load(json_file)
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
ast = model(args=cli(), ws=Workspace('openmp_project'))
# assumes only outermost loops so can get function name easy, double check this
extract_hotspot(ast, hotspot_loop_tag, '_'.join(hotspot_loop_tag.split('_')[:-2]))

# TODO: assumes for loop so can get tag easily
multithread_loop(ast, 'hotspot_for_a', './openmp_project/default', 8)
