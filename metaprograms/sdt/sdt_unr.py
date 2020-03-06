#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

def unroll_loop(ast, loop_tag, unroll_factor=0):

    project = ast.project
    loop = project.query("l:ForLoop{%s}" % loop_tag)[0].l

    # check if there is already a pragma unroll associated with this loop
    pragmas = project.query("g:Global => p:Pragma")
    unroll_pragmas = [row for row in pragmas if row.p.in_code() and 'unroll' in row.p.directive()]
    for row in unroll_pragmas:
        if row.p.next().unparse() == loop.unparse():
            # if yes, modify value
            row.p.instrument(pos='replace', code='#pragma unroll %s' % str(unroll_factor))
            return

    # if not, add a new pragma
    if unroll_factor == 0: # by default, try to fully  unroll
        loop.instrument(pos='before', code='#pragma unroll')
    else:
        loop.instrument(pos='before', code='#pragma unroll %s'% str(unroll_factor))
