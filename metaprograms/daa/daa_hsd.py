#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
import subprocess, os, json

log.level = 2

from plot_hotspot_results import plot_hotspot_results

def instrument_main_function(ast, json):
    # Instrument main function with a timer and report 
    wrap_fn(ast.project, 'main', 'main_', after='Artisan::report("%s");' % json) 
    ast.commit()
    project = ast.project
    main_func = project.query("f:FnDef{main_}")[0].f
    instrument_block(main_func.body(), 'Artisan::Timer __timer__("main", Artisan::op_add);', entry=True) #main_func.body().instrument(pos='begin', code='Artisan::Timer __timer__("main", Artisan::op_add);')
    ast.commit()  

def run_instrumented_code(ast):
    # build and run instrumented code, load reported results 
    ast.export_to("hotspot-result")
    wd = os.getcwd()
    os.chdir("./hotspot-result")
    subprocess.call("make")
    subprocess.call(["make", "run"])
    os.chdir(wd)

def get_hotspots(time_file, threshold):
    # identify hotspots based on threshold
    with open(time_file, 'r') as json_file:
        times = json.load(json_file)
     
    main_time = times["main"][0]
    hotspots = {}
    hotspots["main"] = {"time": main_time, "percentage": 100.0}
    percentages = {}
    for t in times:
        percentage =  (times[t][0]/main_time)*100
        if t != "main" and percentage >= threshold:
            hotspots[t] = {"percentage": percentage}
        percentages[t] = percentage
    
    plot_hotspot_results(percentages, threshold)
    return hotspots

def check_functions(ast, threshold):
    # instrument each functon with  a timer 
    project = ast.project                                 
    func_table = project.query("g:Global => f:FnDef")              
    for row in func_table:
        func = row.f
        if func.in_code() and func.name != "main":
            func.body().instrument(pos='replace', code='{\nArtisan::Timer __timer__("%s", Artisan::op_add); %s' % (func.name, func.body().unparse()[1:])) 

    instrument_main_function(ast, "function_times.json")
    run_instrumented_code(ast)
    
    # get hotspots based on threshold
    return get_hotspots('./hotspot-result/function_times.json', threshold)

def check_loops(ast, threshold):
    
    # instrument each outermost for loop with  a timer 
    project = ast.project                                 
    loop_table = project.query("g:Global => l:ForLoop")              
    for row in loop_table:
        loop = row.l
        if loop.is_outermost(): 
            loop.body().instrument(pos='replace', code='{\nArtisan::Timer __timer__("%s", Artisan::op_add); %s' % (loop.tag(), loop.body().unparse()[1:])) 
    while_table = project.query("g:Global => l:WhileLoop")
    for row in while_table:
        loop = row.l
        if loop.is_outermost(): 
            loop.body().instrument(pos='replace', code='{\nArtisan::Timer __timer__("%s", Artisan::op_add); %s' % (loop.tag(), loop.body().unparse()[1:])) 
    instrument_main_function(ast, "loop_times.json")
    run_instrumented_code(ast)

    # get hotspots based on threshold
    return get_hotspots('./hotspot-result/loop_times.json', threshold)

def hotspot_detection(ast, threshold):

    # Instrument code with artisan include header and init
    project = ast.project
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='begin', code='#define __ARTISAN__INIT__\n#include <artisan.hpp>')
    ast.commit()

    hotspots = check_loops(ast, threshold)

    if  len([t for t in hotspots]) == 1:
        print("No hotspot loops identified, checking functions...")
        # remove loop timers
        ast.undo(sync=True)
        ast.undo(sync=True)
        hotspots = check_functions(ast, threshold)

    with open('./hotspot-result/hotspots.json', 'w') as json_file:
        json.dump(hotspots, json_file)  

    return hotspots

# ast = model(args=cli(), ws=Workspace('daa'))
# hotspots = hotspot_detection(ast, 50.0)
# subprocess.call(['rm', '-rf', 'daa'])