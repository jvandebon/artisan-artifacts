#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess 
import os
import json

log.level = 2

# threshold = 50.0
# ast = model(args=cli(), ws=Workspace('hotspot_check'))
def identify_hotspots(ast, threshold):

    project = ast.project

    # Instrument code with artisan include header and init
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='begin', code='#define __ARTISAN__INIT__\n#include <artisan.hpp>')

    # Find all loops, instrument each with  a timer   
    ## TODO: all loop types?                                  
    loop_table = project.query("g:Global => l:ForLoop")              
    label = 0
    for row in loop_table:
        loop = row.l
        if not loop.body().in_code():
            continue
        # TODO: check if outermost loop 
        if loop.parent().parent().is_entity('ForLoop') or loop.parent().is_entity('ForLoop'):
            continue
        loop.body().instrument(pos='begin', code='Artisan::Timer __timer__("%s", Artisan::op_add);' % loop.tag()) #str(loop_labels[loop.uid]))

    # Instrument main function with a timer and report 
    wrap_fn(project, 'main', 'main_', after='Artisan::report("loop_times.json");') # before='{\nArtisan::Timer __timer__("main", Artisan::op_add);', after='}\nArtisan::report("loop_times.json");')             
    ast.commit()
    project = ast.project
    main_func = project.query("f:FnDef{main_}")[0].f
    main_func.body().instrument(pos='begin', code='Artisan::Timer __timer__("main", Artisan::op_add);')
    ast.commit(sync=False)  

    # build and run instrumented code, load reported results 
    ast.export_to("hotspot-result")
    wd = os.getcwd()
    os.chdir("./hotspot-result")
    subprocess.call("make")
    subprocess.call(["make", "run"])
    os.chdir(wd)
    with open('./hotspot-result/loop_times.json', 'r') as json_file:
        times = json.load(json_file)

    # calculate the time of each loop as a percentage of the main function
    # identify hotspots based on some threshold 
    main_time = times["main"][0]
    hotspots = {}
    hotspots["main"] = {"time": main_time, "percentage": 100.0}
    for t in times:
        percentage =  (times[t][0]/main_time)*100
        if t != "main" and percentage >= threshold:
            hotspots[t] = {"time": times[t][0], "percentage": percentage}

    with open('./hotspot-result/hotspots.json', 'w') as json_file:
        json.dump(hotspots, json_file)
    
    return hotspots
    # print(hotspots, "\n")

