#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *


def next_non_space_char(string, index):
    while(string[index] == " "):
        index += 1
    return index

def remove_dep_with_register(loop, ref):

    loop_body = loop.body().unparse()
    print(loop_body)
    print(ref)

    # instrument before loop with register
    reg = ref['array']+ref['index']+'_reg'
    initialise_reg = ref['type']+" "+reg+ ";\n" + reg + " = " + ref['array'] + '[' + ref['index'] + "];\n"
    loop.instrument(pos='before', code=initialise_reg)

    # replace all references in loop with reg
    lines = loop_body.split('\n')
    for line in lines:
        if ref['array'] in line:
            s = line.index(ref['array'])
            idx = next_non_space_char(line, line.index(ref['array'])+len(ref['array']))
            if line[idx] == '[':
                idx2 = next_non_space_char(line, idx+1)
                if line[idx2:idx2+len(ref['index'])] == ref['index']:
                    idx3 = next_non_space_char(line, idx2+len(ref['index']))
                    if line[idx3] == ']':
                        substring = line[s:idx3+1]
                        line = line.replace(substring, reg)
            # in case there is another occurence
            lines.append(line[s+len(reg):])

    # instrument after loop with assign
    reassign_to_mem = ref['array'] + '[' + ref['index'] + "] = " + reg + ";\n"
    loop.instrument(pos='after', code=reassign_to_mem)   



