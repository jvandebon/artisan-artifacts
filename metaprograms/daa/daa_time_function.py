#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess, os, json

log.level = 2

def time_function(ast, func_name):

    project = ast.project

    # Instrument code with artisan include header and init
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='begin', code='#define __ARTISAN__INIT__\n#include <artisan.hpp>')

    # instument function with timer
    project = ast.project
    func = project.query("f:FnDef{%s}" % func_name)[0].f
    func.body().instrument(pos='begin', code='Artisan::Timer __timer__("%s", Artisan::op_add);' % func_name)

    # instrument main function with report 
    main_func = project.query("f:FnDef{main}")[0].f
    instrument_block(main_func.body(), 'Artisan::report("%s_time.json");' % func_name, exit=True)

    ast.commit()
    
    # build and run instrumented code, load reported results 
    ast.export_to("time_function")
    wd = os.getcwd()
    os.chdir("./time_function")
    subprocess.call("make")
    subprocess.call(["make", "run"])
    os.chdir(wd)
    with open('./time_function/%s_time.json' % func_name, 'r') as json_file:
        times = json.load(json_file)

    print(times[func_name])
    return times[func_name]

ast = model(args=cli(), ws=Workspace('temp'))
time_function(ast, 'hotspot')
subprocess.call(['rm', '-rf', 'temp'])