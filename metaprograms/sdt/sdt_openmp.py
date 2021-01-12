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
        print("Can't determine if loop is parallel : %s" % loop_tag)
        return
    directive = parallel_pragmas[0].p.directive()
    parallel = json.loads(' '.join(directive.split()[directive.split().index('parallel') + 1:]))
    if parallel['is_parallel'] != 'True':
        print("Cannot parallelise loop with OpenMP : %s" % loop_tag)
        return

    # check if there is already a pragma unroll associated with this loop
    pragmas = project.query("g:Global => p:Pragma")
    for row in pragmas:
        if row.p.in_code() and 'omp parallel for' in row.p.directive() and row.p.next().unparse() == loop.unparse():
            # if yes, modify value
            row.p.instrument(pos='replace', code='#pragma omp parallel for num_threads(%s)' % str(num_threads))
            ast.commit()
            return

    # add #include <omp.h>
    global_scope = project.query("g:Global")
    global_scope[0].g.instrument(pos='begin', code='#include <omp.h>')

    # insert pragma above loop
    loop.instrument(pos='before', code='#pragma omp parallel for num_threads(%s)' % str(num_threads))

    # insert define at top of code 
    # global_scope[0].g.instrument(pos='begin', code='#define NUM_THREADS %s' % str(num_threads))
    ast.commit()

    # modify Makefile flags to include -fopenmp
    with open(path_to_source + '/Makefile', 'r') as makefile:
        contents = makefile.read()
    if not '-fopenmp' in contents:    
        i = contents.index('FLAGS=')
        contents = contents[:i+6] + "-fopenmp " + contents[i+6:]
        with open(path_to_source + '/Makefile', 'w') as makefile:
            makefile.write(contents)
