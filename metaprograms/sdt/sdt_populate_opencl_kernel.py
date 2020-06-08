#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

def format_function_args(args):
    formatted_args = []
    for arg in args:
        if "*" in arg['string']:
            formatted_args.append("__global " + arg['string'])
        else:
            formatted_args.append(arg['string'])
    return ', '.join(formatted_args)
# ********************************************************************************
# populate kernel.cl file with call to lobrary function / correct args 
# ********************************************************************************
# TODO: copy over all struct and constant definitions / create a common header file? 
def populate_opencl_kernel(path_to_kernel, args, header):
    # function arguments for extern library function declaration
    opencl_func_args = format_function_args(args)
    # kernel argument
    opencl_kernel_args = opencl_func_args.replace("*", "* restrict ")
    # runtime arguments passed to library function 
    opencl_runtime_args = ' ,'.join([a['name'] for a in args])
    # write to template kernel.cl
    with open(path_to_kernel, 'r') as f:
        f_contents = f.read()    
    if "*FUNC_ARGS*" in f_contents:
        f_contents = f_contents.replace("*FUNC_ARGS*", opencl_func_args)
    if "*KERNEL_ARGS*" in f_contents:
        f_contents = f_contents.replace("*KERNEL_ARGS*", opencl_kernel_args)
    if "*RUNTIME_ARGS*" in f_contents:
        f_contents = f_contents.replace("*RUNTIME_ARGS*", opencl_runtime_args)
    for h in header:
        f_contents = "#include \"" + h + "\"\n" + f_contents

    with open(path_to_kernel, 'w') as f:
        f.write(f_contents)
    
    return opencl_kernel_args