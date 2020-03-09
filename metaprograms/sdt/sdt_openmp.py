#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import json

def multithread_loop(ast, loop_tag, path_to_source, num_threads):

    project = ast.project

    # find loop based on input 
    # TODO: not only for loops
    loop = project.query("l:ForLoop{%s}" % loop_tag)[0].l

    # verify that loop is parallel
    parallel_pragmas = [row for row in loop.query("p:Pragma") if 'artisan-hls' in row.p.directive() and 'parallel' in row.p.directive()]
    if len(parallel_pragmas) == 0:
        print("Can't determine if loop is parallel.")
        exit(0)
    directive = parallel_pragmas[0].p.directive()
    parallel = json.loads(' '.join(directive.split()[directive.split().index('parallel') + 1:]))
    if parallel['is_parallel'] != 'True':
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
