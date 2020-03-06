#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

log.level = 2

def isParallel(loop):
    return True

def multithread_loop(ast, loop_tag, path_to_source, num_threads):

    project = ast.project

    # find loop based on input 
    # TODO: not only for loops
    loop = project.query("l:ForLoop{%s}" % loop_tag)[0].l

    # TODO: verify if parallel
    if not isParallel(loop):
        print("Cannot parallelise loop with OpenMP.")
        exit(0)

    # add #include <omp.h>
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='begin', code='#include <omp.h>')

    # insert pragma above loop
    loop.instrument(pos='before', code='#pragma omp parallel for num_threads(NUM_THREADS)')

    # insert define at top of code 
    global_scope[0].g.instrument(pos='begin', code='#define NUM_THREADS %s' % str(num_threads))
    ast.commit()

    # modify Makefile flags to include -fopenmp
    with open(path_to_source + '/Makefile', 'r') as makefile:
        contents = makefile.read()
    if not '-fopenmp' in contents:    
        i = contents.index('FLAGS=')
        contents = contents[:i+6] + "-fopenmp " + contents[i+6:]
        with open(path_to_source + '/Makefile', 'w') as makefile:
            makefile.write(contents)
    

