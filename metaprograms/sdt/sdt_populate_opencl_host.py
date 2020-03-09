#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess, os
import opencl_host_strings

def populate_opencl_host(ast, path_to_source, path_to_host, kernel_args):

    project = ast.project

    # find all calls to malloc and  free, isntrument with hardware version, check size for malloc
    # TODO: better way to get pointer sizes
    calls = project.query("g:Global => c:ExprCall")
    for row in calls:
        if row.c.in_code() and row.c.name() == 'free':
            row.c.stmt().instrument(pos='before', code='#ifdef HW\n' + row.c.stmt().unparse().replace("free", "aocl_utils::alignedFree") + "\n#else\n")
            row.c.stmt().instrument(pos='after', code='#endif')
        if row.c.in_code() and row.c.name() == 'malloc':
            stmt = row.c.stmt().unparse()
            s = stmt.index('malloc')+6
            size = stmt[s:].replace(';','').strip()[:-1]
            a = row.c.parent().parent().parent().unparse().strip()
            for arg in kernel_args:
                if arg['name'] == a:
                    arg['size'] = size
            row.c.stmt().instrument(pos='before', code='#ifdef HW\n' + row.c.stmt().unparse().replace("malloc", "aocl_utils::alignedMalloc") + "\n#else\n")
            row.c.stmt().instrument(pos='after', code='#endif')

 
    # find all new / delete
    news = project.query("g:Global => n:ExprNew")
    for row in news:
        if row.n.in_code():
            a = row.n.parent().parent().unparse().strip()
            type_str = row.n.type().unparse()
            ptr_type = type_str.split()[0]
            size = type_str[type_str.index('[')+1:type_str.index(']')] + " * sizeof(" + ptr_type + ")"
            # TODO: better way to get parent statement?
            # if row.n.parent().parent().parent().is_entity('Stmt'):
                # print('found statement')
                # stmt = row.n.parent().parent().parent()
            stmt = row.n.stmt()
            malloc_string = ptr_type + " *" + a + " = (" + ptr_type + " *) aocl_utils::alignedMalloc(" + size + ");"
            # print(malloc_string)
            stmt.instrument(pos='before', code='#ifdef HW\n' + malloc_string + "\n#else\n")
            stmt.instrument(pos='after', code='#endif')
            for arg in kernel_args:
                if arg['name'] == a:
                    arg['size'] = size
    deletes = project.query("g:Global => d:ExprDelete")
    for row in deletes:
        if row.d.in_code():
            free_string = "aocl_utils::alignedFree(" + row.d.var().unparse().strip() + ");"
            # TODO: better way to get parent statement?
            
            row.d.stmt().instrument(pos='before', code='#ifdef HW\n' + free_string + "\n#else\n")
            row.d.stmt().instrument(pos='after', code='#endif')

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

    # createbuffers for kernel pointer args
    cl_rw = {'R': "CL_MEM_READ_ONLY", 'RW': "CL_MEM_READ_WRITE", 'W': "CL_MEM_WRITE_ONLY"}
    create_buffers = "cl_int status1;\n"
    buffers_to_release = []
    for arg in kernel_args:
        if "*" in arg['type']:
            create_buffers += arg['name'] + "_buffer = clCreateBuffer(context, " + cl_rw[arg['rw']] + ", " + arg['size'] + ", 0, &status1);\n" 
            create_buffers += 'checkError(status1, "Failed to create ' + arg['name'] + ' buffer.");\n'
            buffers_to_release.append(arg['name'] + "_buffer")

    # enqueue write buffers
    enqueue_write_buffers = "cl_int status2;\n"
    for arg in kernel_args:
        if "*" in arg['type'] and "R" in arg['rw']:
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
        if "*" in arg['type'] and "W" in arg['rw']:
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
