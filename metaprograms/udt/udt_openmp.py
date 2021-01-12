#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys, os, json, subprocess

sys.path.insert(1, '/workspace/metaprograms/sdt/')
sys.path.insert(2, '/workspace/metaprograms/daa/')

from daa_hsd import hotspot_detection
from sdt_extract_hotspot import extract_hotspot_loop_to_function
from sdt_openmp import multithread_loop
from daa_time_function import time_function
from daa_time_function import remove_timer
## UDT to optimise by multithreading with OpenMP

def get_called_funcs(project, func, results):
    called_funcs = func.query("cf:ExprCall")
    for row in called_funcs:
        new_func = row.cf
        if new_func.name() not in [f.name for f in results]:
            function_table = project.query("g:Global => f:FnDef", where="f.name == '%s'" % new_func.name())
            if len(function_table) == 0:
                continue
            for row in function_table:
                function = row.f
                if function and function.in_code():
                    results.append(function)
                    get_called_funcs(project, function,results)

def remove_artisan_pragmas(ast):
    project = ast.project
    pragmas = project.query("g:Global => p:Pragma")
    for row in pragmas:
        if row.p.in_code() and 'artisan-hls' in row.p.directive():
            row.p.instrument(pos='replace', code='')
   
    ast.commit()
# identify hotspot loop for acceleration
if os.path.exists("./hotspot-result"):
    with open('./hotspot-result/hotspots.json', 'r') as json_file:
        hotspots= json.load(json_file)
else:
    daa_ast = model(args=cli(), ws=Workspace('daa'))
    hotspots = hotspot_detection(daa_ast, 50.0)
    subprocess.call(['rm', '-rf', 'daa'])



hotspots = [tag for tag in hotspots if tag != 'main']
if len(hotspots) > 1:
    print("More than one hotspot identified.")
    # TODO: get user input / further analysis
    # exit(0)
elif len(hotspots) == 0:
    print("No hotspot identified for acceleration.")
    # TODO: get user input / further analysis
    exit(0)
else:
    if '_for_' in hotspots[0]:
        hotspot_loop_tag = hotspots[0]
        hotspot_func = False
    else:
        hotspot_func = hotspots[0]

hotspot_func = "rendering_sw"

# extract hotspot
ast = model(args=cli(), ws=Workspace('openmp_ws'))

if hotspot_func == False:
    # assumes only outermost loops so can get function name easy, double check this
    extract_hotspot_loop_to_function(ast, hotspot_loop_tag, '_'.join(hotspot_loop_tag.split('_')[:-2]))
    hotspot_func = "hotspot"
    # TODO: assumes for loop 
    tags = ['hotspot_for_a']
else:
    # find outermost for loops in hotspot function
    print("searching for outermost loops...")
    tags = []
    func = ast.project.query("f:FnDef{%s}" % hotspot_func)[0].f
    loop_table = func.query("l:ForLoop")
    for row in loop_table:
        if row.l.is_outermost():
            tags.append(row.l.tag())
    called_funcs = []
    get_called_funcs(ast.project, func, called_funcs)
    for f in called_funcs:
        loop_table = f.query("l:ForLoop")
        for row in loop_table:
            if row.l.is_outermost():
                tags.append(row.l.tag())
    if len(tags) == 0:
        print("No loops to multithread.")
        exit(0)



# ast.export_to('extracted_hotspot')
# DSE 
NUM_THREADS = 2
time = 100000
prev_time = 1000000
while time < prev_time*0.9:
    prev_time = time
    for t in tags:
        multithread_loop(ast, t, './openmp_ws/default', NUM_THREADS)
    time = time_function(ast, hotspot_func)[0]
    print('Trying', NUM_THREADS, 'threads, time:', time)
    NUM_THREADS *= 2

# ROLLBACK
NUM_THREADS = int(NUM_THREADS/4)
for t in tags:
    multithread_loop(ast, t, './openmp_ws/default', NUM_THREADS)
print('Finished  DSE, rolling back to', NUM_THREADS, 'threads')
remove_timer(ast, 'hotspot')

# instrument code with human readable timer
before_hotspot_call = "struct timeval t1, t2;\ngettimeofday(&t1, NULL);\n"
after_hotspot_call = "gettimeofday(&t2, NULL);\nprintf(\"CPU computation took %lfs\", (double)(t2.tv_usec-t1.tv_usec)/1000000 + (double)(t2.tv_sec-t1.tv_sec));\n"
# find call to hotspot
calls = [row.c for row in ast.project.query("g:Global => c:ExprCall") if row.c.in_code() and row.c.name() == hotspot_func]

for c in calls:
    print(c)
    c.instrument(pos='before', code=before_hotspot_call)
    c.parent().instrument(pos='after', code=after_hotspot_call)

ast.commit()

ast.export_to('openmp_project')
subprocess.call(['cp', './openmp_ws/default/Makefile', './openmp_project'])
subprocess.call(['rm', '-rf', 'openmp_ws'])
subprocess.call(['rm', '-rf', 'time_function'])
