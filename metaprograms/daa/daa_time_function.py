#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess, os, json

log.level = 2

def time_function(ast, func_name):

    project = ast.project

    # Instrument code with artisan include header and init
    func = project.query("f:FnDef{%s}" % func_name)[0].f
    if 'Artisan::Timer' not in func.body().unparse():
        global_scope = project.query("g:Global")
        global_scope[0].g.instrument(pos='begin', code='#define __ARTISAN__INIT__\n#include <artisan.hpp>')

    # instument function with timer
    project = ast.project
    func = project.query("f:FnDef{%s}" % func_name)[0].f
    if 'Artisan::Timer' not in func.body().unparse():
        func.body().instrument(pos='begin', code='Artisan::Timer __timer__("%s", Artisan::op_add);' % func_name)

        # instrument main function with report 
        main_func = project.query("f:FnDef{main}")[0].f
        instrument_block(main_func.body(), 'Artisan::report("%s_time.json");' % func_name, exit=True)

        ast.commit()
    
    # build and run instrumented code, load reported results 
    ast.export_to("time_function")
    wd = os.getcwd()
    os.chdir("./time_function")
    subprocess.call(["make", "clean"])
    subprocess.call("make")
    subprocess.call(["make", "run"]
    os.chdir(wd)
    with open('./time_function/%s_time.json' % func_name, 'r') as json_file:
        times = json.load(json_file)

    return times[func_name]

def remove_timer(ast, func_name):
    
    # instument function with timer
    project = ast.project
    func = project.query("f:FnDef{%s}" % func_name)[0].f
    new_body = func.body().unparse().replace('class Artisan::Timer __timer__(("%s"),Artisan::op_add);' % func_name, '')
    func.body().instrument(pos='replace', code=new_body)

    # instrument main function with report 
    main_func = project.query("f:FnDef{main}")[0].f
    new_main = main_func.body().unparse().replace('Artisan::report("%s_time.json");' % func_name, '')
    main_func.body().instrument(pos='replace', code=new_main)

    ast.commit()

    # Instrument code with artisan include header and init
    project = ast.project
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='replace', code=global_scope[0].g.unparse().replace('#define __ARTISAN__INIT__\n#include <artisan.hpp>', ''))

    ast.commit()

ast = model(args=cli(), ws=Workspace('temp'))
time_function(ast, 'run_cpu')
subprocess.call(['rm', '-rf', 'temp'])

