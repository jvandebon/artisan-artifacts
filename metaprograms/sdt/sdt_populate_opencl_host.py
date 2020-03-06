#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess 
import os

import opencl_host_strings

def populate_opencl_host(ast, path_to_source, path_to_host, kernel_args):

    project = ast.project

    # find all malloc/new calls, determine arg size, instrument code 
    # TODO: handle static allocation 
    for arg in [p for p in kernel_args if "*" in p['type']]:    
        vd_table = project.query("vd:VarDef", where="vd.name == '%s'" % arg['name'])
        for row in vd_table:
            decl = row.vd.decl().unparse()
            if "malloc" in decl:
                idx = decl.index("malloc") + 6
                substring = decl[idx:]
                size = substring[substring.find("("):substring.rfind(")")]
                arg['size'] = size
                row.vd.decl().instrument(pos='before', code='#ifdef HW\n' + decl.replace("malloc", "aocl_utils::alignedMalloc") + "\n#else\n")
                row.vd.decl().instrument(pos='after', code='#endif')
            elif "new" in decl:
                s = decl.index('[') +1
                e = decl.index(']')
                arg['size'] = decl[s:e] + "*sizeof(" + row.vd.type().unparse().replace('*', '').strip() + ")"
                alloc_string = arg['type'] + " " + arg['name'] + " = " + "(" + arg['type'] + ") aocl_utils::alignedMalloc(" + arg['size'] + ");\n"
                row.vd.decl().instrument(pos='before', code='#ifdef HW\n' + alloc_string + "\n#else\n")
                row.vd.decl().instrument(pos='after', code='#endif')


    # same for free
    calls = project.query("g:Global => c:Call")
    for row in calls:
        if row.c.in_code() and row.c.name() == 'free':
            row.c.stmt().instrument(pos='before', code='#ifdef HW\n' + row.c.stmt().unparse().replace("free", "aocl_utils::alignedFree") + "\n#else\n")
            row.c.stmt().instrument(pos='after', code='#endif')
    
    # for delete, need statements
    # TODO: better way  to find delete
    exprstmts = project.query("g:Global => s:ExprStmt")
    for row in exprstmts:
        if row.s.in_code() and 'delete' in row.s.unparse(): 
            for a in [a['name'] for  a in kernel_args]:
                if a in row.s.unparse():
                    free_string = "aocl_utils::alignedFree(" + a + ");"
                    row.s.instrument(pos='before', code='#ifdef HW\n' + free_string + "\n#else\n")
                    row.s.instrument(pos='after', code='#endif')

    # generate all opencl string blocks, find the right places in the code to insert 
    # define buffers for kernel args
    # TODO: support more than cl_int and cl_mem types
    buffers_to_initialise = []
    declare_buffers = "#ifdef HW\n"
    for arg in kernel_args:
        if "*" in arg['type']:
            declare_buffers += "static cl_mem " + arg['name'] + "_buffer = NULL;\n"
        elif "int" in arg['type']:
            declare_buffers += "static cl_int " + arg['name'] + "_buffer;\n"
            buffers_to_initialise.append(arg['name'])
    declare_buffers += "#endif\n"

    # insert PROG_START and buffer declarations above main
    global_ =  project.query("g:Global")[0].g
    global_.instrument(pos='before', code='#include <sys/time.h>\n' + opencl_host_strings.PROG_START + declare_buffers)

    # insert MAIN_START at beginning of main 
    main_func =  project.query("g:Global =>  f:FnDef{main}")[0].f
    main_func.body().instrument(pos='begin', code=opencl_host_strings.MAIN_START)

    #  initialise cl_int buffers
    initialise_cl_ints = ""
    for buffer in buffers_to_initialise:
        initialise_cl_ints +=  buffer + "_buffer = " + buffer  +  ";\n"

    # create write buffers for kernel pointer args
    create_buffers = "cl_int status1;\n"
    buffers_to_release = []
    for arg in kernel_args:
        if "*" in arg['type'] and "READ" in arg['rw']:
            create_buffers += arg['name'] + "_buffer = clCreateBuffer(context, " + arg['rw'] + ", " + arg['size'] + ", 0, &status1);\n" 
            create_buffers += 'checkError(status1, "Failed to create ' + arg['name'] + ' buffer.");\n'
            buffers_to_release.append(arg['name'] + "_buffer")

    # enqueue write buffers
    enqueue_write_buffers = "cl_int status2;\n"
    for arg in kernel_args:
        if "*" in arg['type']:
            enqueue_write_buffers += "status2 = clEnqueueWriteBuffer(queue," + arg['name'] + "_buffer,0,0," + arg['size'] + "," + arg['name'] + ",0,0,0);\n"
            enqueue_write_buffers += 'checkError(status2, "Failed to enqueue write buffer ' + arg['name'] + '");\n'

    # set kernel args
    set_kernel_args = "cl_int status3;\n"
    for i in range(0, len(kernel_args)):
        arg = kernel_args[i]
        if "*" in arg['type']:
            set_kernel_args += "status3 = clSetKernelArg(kernel_lib," + str(i) + ",sizeof(cl_mem),&" + arg['name'] + "_buffer);\n"
        elif "int" in arg['type']:
            set_kernel_args += "status3 = clSetKernelArg(kernel_lib," + str(i) + ",sizeof(cl_int),&" + arg['name'] + "_buffer);\n"
        set_kernel_args += 'checkError(status3, "Failed to set kernel_lib arg ' + str(i) + '");\n'

    # enqueue read buffers
    enqueue_read_buffers = "cl_int status4;\n"
    for arg in kernel_args:
        if "*" in arg['type'] and "WRITE" in arg['rw']:
            enqueue_read_buffers += "status4 = clEnqueueReadBuffer(queue, " + arg['name'] + "_buffer, 1, 0, " + arg['size'] + ", " + arg['name'] + ", 0, 0, 0);\n"
            enqueue_read_buffers += 'checkError(status4, "Failed to enqueue read buffer ' + arg['name'] + '");\n'

    before_hotspot_call  = '#ifdef HW\n'
    before_hotspot_call += initialise_cl_ints
    before_hotspot_call += create_buffers
    before_hotspot_call += enqueue_write_buffers
    before_hotspot_call += set_kernel_args
    before_hotspot_call += opencl_host_strings.ENQUEUE_KERNEL
    before_hotspot_call += enqueue_read_buffers
    before_hotspot_call += '#else\nprintf("Running on CPU ...");\nstruct timeval t1, t2;\ngettimeofday(&t1, NULL);\n'
    after_hotspot_call = 'gettimeofday(&t2, NULL);\n'
    after_hotspot_call += 'printf("CPU computation took %lfs", (double)(t2.tv_usec-t1.tv_usec)/1000000 + (double)(t2.tv_sec-t1.tv_sec));\n'
    after_hotspot_call += '#endif\n'

   # insert before call to hotspot (instrument with timer)
    for row in calls:
        if row.c.in_code() and row.c.name() == 'hotspot':
            row.c.stmt().instrument(pos='before', code=before_hotspot_call)
            row.c.stmt().instrument(pos='after', code=after_hotspot_call)

    # modify cleanup function definition to release created buffers, insert definition, call before end of main   
    release_buffers = ""
    for buffer in buffers_to_release:
        release_buffers += "if(" + buffer + "){ clReleaseMemObject(" + buffer + "); }\n"

    cleanup_function = opencl_host_strings.CLEANUP.replace('*RELEASE_BUFFERS*', release_buffers)
    main_func.instrument(pos='after', code=cleanup_function)
    instrument_block(main_func.body(), "#ifdef  HW\ncleanup();\n#endif", exit=True)
    
    ast.commit()
    for f in os.listdir(path_to_source):
        if f.endswith(".cpp"):
            subprocess.call(['cp', path_to_source + "/" + f, path_to_host])