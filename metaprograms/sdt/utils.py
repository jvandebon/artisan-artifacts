#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *
import json, os, subprocess

def determine_args(params, vars_rw, pointer_sizes):
    # determine args, types, sizes if static 
    args = []
    for p in params:
        arg = {}
        arg['name'] = p.name
        arg['type'] = p.type().unparse().strip()

        if arg['name'] in vars_rw:
            arg['rw'] = vars_rw[arg['name']]
        else:
            arg['rw'] = 'RW'

        arg['dims'] = [] 

        if "[]" in arg['type']:
            arg['size_string'] = "sizeof(%s)" % arg['type'].split('[')[0].strip().replace("const", "")
            arg['large'] = False
            arg['is_pointer'] = True
            arg['pointer_type'] = arg['type'].split('[')[0].strip().replace("const", "") + " *"

        elif '[' in arg['type'] and ']' in arg['type']: # fixed length array
            tmp = '['.join(arg['type'].split('[')[1:]).strip()
            # handle 1D and 2D arrays
            size = 1
            for val in tmp.split('['):
                try:
                    arg['dims'].append(int(val[:-1]))
                    size *= int(val[:-1])
                except:
                    arg['dims'] = []
                    break
            arg['size_string'] = str(size) +"*sizeof(%s)" % arg['type'].split('[')[0].strip().replace("const", "")
            arg['is_pointer'] = True
            arg['pointer_type'] = arg['type'].split('[')[0].strip().replace("const", "") + " *"
            if size > 2**20:    # TODO: arbitrary 
                arg['large'] = True
            else:
                arg['large'] = False

        elif "*" in arg['type']: # pointer type 
            arg['is_pointer'] = True
            arg['pointer_type'] = arg['type']
            if arg['name'] in pointer_sizes: # known size 
                size = pointer_sizes[arg['name']]
                arg['size_string'] = size+"*sizeof("+arg['type'].split()[0]+")"
                arg['dims'].append(int(size))
                if int(size) > 2**20:    # TODO: arbitrary 
                    arg['large'] = True
                else:
                    arg['large'] = False
            else:
                arg['large'] = False          
                arg['size_string'] = ""
            

        else: # not an array or pointer
            arg['size_string'] = ""
            arg['large'] = False
            arg['is_pointer'] = False
                
        args.append(arg)  
    # for a in args:
    #     print(a)
    return args 

# def determine_args(params, vars_rw, pointer_sizes,kernel_args ):
#     # determine args, types, sizes if static 
#     args = []
#     for p in params:
#         arg = {}
#         arg['name'] = p.name
#         arg['type'] = p.type().unparse().strip()
#         arg['dims'] = []

#         if arg['name'] in vars_rw:
#             arg['rw'] = vars_rw[arg['name']]
#         else:
#             arg['rw'] = 'RW' 
        
#         if '[' in arg['type'] and ']' in arg['type']: # fixed length array
#             if "[]" in arg['type']:
#                 arg['size'] = ""
#             arg['string'] = arg['type'].split('[')[0].strip() + " " + arg['name'] + '[' + '['.join(arg['type'].split('[')[1:]).strip()
#             arg['kernel_string'] = arg['type'].split('[')[0].strip() + " * _" + arg['name']
#             # handle 1D and 2D arrays
#             tmp = arg['string'][arg['string'].index('[')+1:]
#             size = 1
#             for val in tmp.split('['):
#                 try:
#                     arg['dims'].append(int(val[:-1]))
#                     size *= int(val[:-1])
#                 except:
#                     arg['dims'] = []
#                     arg['size'] = ""
#                     arg['string'] = arg['kernel_string']
#             arg['size'] = str(size) +"*sizeof(%s)" % arg['type'].split('[')[0].strip().replace("const", "")

#         elif "*" in arg['type']: # pointer type 
#             if arg['name'] in pointer_sizes: # known size 
#                 arg['size'] = pointer_sizes[arg['name']]+"*sizeof("+arg['type'].split()[0]+")"
#                 arg['dims'].append(int(pointer_sizes[arg['name']]))
#                 arg['kernel_string'] = arg['type'].split('[')[0].strip() + " _" + arg['name']
#                 arg['string'] = arg['type'].replace("*", "") + " " + arg['name'] + "[" + pointer_sizes[arg['name']] + "]"
#             else:
#                 arg['string'] = arg['type'] + " " + arg['name']
#                 arg['kernel_string'] = arg['string']
#                 arg['size'] = ""


#         else: # not an array or pointer
#             arg['string'] = arg['type'] + " " + arg['name']
#             arg['kernel_string'] = arg['string']
#             arg['size'] = ""

#         # if fixed size but too big for local memory, make pointer arg, flag as 'large'
#         # TODO: arbitrary 
#         if len(arg['dims']) != 0 and arg['dims'][0] > 2**20: 
#             arg['large'] = True
#             arg['kernel_string'] = arg['type'].split('[')[0].strip() + " * " + arg['name']
#         else:
#             arg['large'] = False        
        
#         args.append(arg)   
#     return args 

# TODO: be careful to check types of params / signatures match (right now just checks number of args)
def get_called_funcs(project, func, results):
    called_funcs = func.query("cf:ExprCall")
    for row in called_funcs:
        new_func = row.cf
        num_args = len(new_func.children()[-1].unparse().strip().split(","))
        if new_func.name() not in [f[0].name for f in results]:
            function_table = project.query("g:Global => f:FnDef", where="f.name == '%s'" % new_func.name())
            if len(function_table) == 0:
                continue
            for row in function_table:
                function = row.f
                if (function, function.decl().params()) in results:
                    continue
                if function and function.in_code():
                    num_params = len(function.decl().params())
                    # print(function.name, ", num params:", num_params)
                    if num_params == num_args:
                        results.append((function,function.decl().params(),func.name,new_func.unparse().strip()))
                        get_called_funcs(project, function,results)

def copy_header_files(header, template_base):
    for h in header:
        if 'directory' in h and 'host_only' in h:
            subprocess.call(['cp','-r', h['path']+h['file'], template_base + 'project/host/src/'+h['file']+"/"])
            continue
        subprocess.call(['cp', h['path']+h['file'], template_base + 'project/host/src/'])
        if 'host_only' in h:
            continue
        subprocess.call(['cp', h['path']+h['file'], template_base ])
        subprocess.call(['cp', h['path']+h['file'], template_base + 'project/device/'])

### ANALYSIS PRAGMA INTERFACES ###

def is_loop_parallel(loop):
    parallel_pragmas = [row for row in loop.query("p:Pragma") if 'artisan-hls' in row.p.directive() and 'parallel' in row.p.directive()]
    if len(parallel_pragmas) == 0:
        print("Can't determine if loop is parallel.")
        exit(0)
    directive = parallel_pragmas[0].p.directive()
    parallel = json.loads(' '.join(directive.split()[directive.split().index('parallel') + 1:]))
    if parallel['is_parallel'] != 'True':
        print("Cannot parallelise loop.")
        exit(0)
    return True


def check_vars_rw(func):
    var_pragmas = [row for row in func.query("p:Pragma") if 'artisan-hls' in row.p.directive() and 'vars' in row.p.directive()]
    if len(var_pragmas) > 0:
        directive = var_pragmas[0].p.directive()
        vars_rw = json.loads(' '.join(directive.split()[directive.split().index('vars') + 1:]))
    else:
        vars_rw = {}
    return vars_rw

def check_pointer_sizes(func):
    pointer_pragmas = [row for row in func.query("p:Pragma") if 'artisan-hls' in row.p.directive() and 'pointers' in row.p.directive()]
    if len(pointer_pragmas) > 0:
        directive = pointer_pragmas[0].p.directive()
        pointer_sizes = json.loads(' '.join(directive.split()[directive.split().index('pointers') + 1:]))
    else:
        pointer_sizes = {}
    return pointer_sizes