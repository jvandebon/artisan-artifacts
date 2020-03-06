#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import sys

# inputs: ast, loop to be extracted, function 
def extract_hotspot(ast, hotspot_loop_tag, hotspot_loop_func):

    project = ast.project

    # find loop based on input 
    # TODO: not only for loops
    loop = project.query("l:ForLoop{%s}" % hotspot_loop_tag)[0].l

    # work out all local variables
    local_vars = []
    vardefs = loop.query("vd:VarDef")
    for row in vardefs:
        local_vars.append(row.vd.name)

    # check all remaining statements/expressions for references to any other variable
    exprs = loop.query("e:Expr")
    other_vars = []
    arguments = []
    bad_parent_types = ['SgFunctionCallExp', 'SgDotExp']
    for row in exprs:
        if len(row.e.children()) == 0:
            if row.e.unparse().strip() not in local_vars and not row.e.is_val() and not row.e.parent().sg_type_real in bad_parent_types:
                a = row.e.unparse().strip()
                # TODO: check for constants in a better way 
                if a.isupper():
                    continue
                if a not in other_vars:
                    row.e.instrument(pos='replace', code=row.e.unparse())
                    other_vars.append(a)
                    type_str = row.e.type().unparse()
                    # TODO: handle fixed length lists better
                    if('[' in type_str.split()[-1] and ']' in type_str.split()[-1]):
                        arguments.append(type_str.split('[')[0].strip() + " " + a + '[' + '['.join(type_str.split('[')[1:]).strip())
                    else:
                        arguments.append(type_str.strip() + " " + a)

    # create new function containing loop body with determined args
    hotspot_function_string = "void hotspot(" + ', '.join(arguments) + ") {\n " + loop.unparse() +"\n}"
    parent_function = project.query("g:Global => f:FnDef{%s}" % hotspot_loop_func)[0].f 
    parent_function.instrument(pos='before', code=hotspot_function_string)

    # replace loop body in source with call to the new function
    loop.instrument(pos='replace', code="hotspot(" + ', '.join(other_vars)+ ");\n")

    ast.commit()
    # ast.export_to("extracted_hotspot")


# hotspot_loop_tag = 'rendering_sw_for_a' #'SgdLR_sw_for_a' #'main_for_b' 'compute_for_a' 'run_cpu_for_a'
# hotspot_loop_func = 'rendering_sw' #'SgdLR_sw' #'main' 'compute' 'run_cpu'

# extract_hotspot(sys.argv[1], hotspot_loop_tag, hotspot_loop_func)