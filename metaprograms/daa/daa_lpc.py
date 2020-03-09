#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
log.level = 2
import subprocess, os, json

def get_loop_counts(ast):

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
        loop.body().instrument(pos='begin', code='Artisan::monitor("%s", 1, Artisan::op_add);' % loop.tag())

    # Instrument main function with report 
    main_func = project.query("f:FnDef{main}")[0].f
    instrument_block(main_func.body(), 'Artisan::report("loop_counts.json");', exit=True)
    ast.commit()  

    # build and run instrumented code, load reported results 
    ast.export_to("loopcount-result")
    wd = os.getcwd()
    os.chdir("./loopcount-result")
    subprocess.call("make")
    subprocess.call(["make", "run"])
    os.chdir(wd)
    with open('./loopcount-result/loop_counts.json', 'r') as json_file:
        counts = json.load(json_file)

    print(counts)
    # # calculate the time of each loop as a percentage of the main function
    # # identify hotspots based on some threshold 
    # main_time = times["main"][0]
    # hotspots = {}
    # hotspots["main"] = {"time": main_time, "percentage": 100.0}
    # for t in times:
    #     percentage =  (times[t][0]/main_time)*100
    #     if t != "main" and percentage >= threshold:
    #         hotspots[t] = {"time": times[t][0], "percentage": percentage}

    # with open('./hotspot-result/hotspots.json', 'w') as json_file:
    #     json.dump(hotspots, json_file)
    
    # return hotspots
    # print(hotspots, "\n")

ast = model(args=cli(), ws=Workspace('lc_check'))
get_loop_counts(ast)


