#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

import subprocess, os
import opencl_host_strings

def instrument_dynamic_memory_allocation(project, kernel_args):
    # malloc / free 
    calls = project.query("g:Global => c:ExprCall")
    for row in calls:
        if row.c.in_code() and row.c.name() == 'free':
            row.c.stmt().instrument(pos='before', code='#ifdef HW\n' + row.c.stmt().unparse().replace("free", "aocl_utils::alignedFree") + "\n#else\n")
            row.c.stmt().instrument(pos='after', code='#endif')
        elif row.c.in_code() and row.c.name() == 'malloc':
            stmt = row.c.stmt().unparse()
            s = stmt.index('malloc')+6
            e = stmt.rfind(')')
            size = stmt[s:e].replace(';','').strip()
            
            # TODO: handle all mismatched bracket cases
            while size.count(")") > size.count("("):
                size = size[:size.rfind(")")]
            a = ""
            if row.c.parent().parent().parent().sg_type_real == 'SgInitializedName':
                a = row.c.parent().parent().parent().unparse().strip()
            elif len(stmt.split("=")) == 2 and 'malloc' in stmt.split("=")[1]:
                a = stmt.split("=")[0].strip()
            for arg in kernel_args:
                if arg['name'] == a:
                    arg['size_string'] = size
                    arg['allocated'] = True
            row.c.stmt().instrument(pos='before', code='#ifdef HW\n' + row.c.stmt().unparse().replace("malloc", "aocl_utils::alignedMalloc") + "\n#else\n")
            row.c.stmt().instrument(pos='after', code='#endif')
        #BUG: GET RID OF STD::
        elif row.c.in_code() and "std::" in row.c.stmt().unparse().strip():
            row.c.stmt().instrument(pos='replace', code=row.c.stmt().unparse().replace("std::", ""))

    # new / delete
    news = project.query("g:Global => n:ExprNew")
    for row in news:
        if row.n.in_code():

            a = row.n.parent().parent().unparse().strip()
            type_str = row.n.unparse().replace("new", "").strip()
            ptr_type = type_str.split()[0]
            #TODO: doesn't handle pointers to pointers
            size = type_str[type_str.index('[')+1:type_str.index(']')] + " * sizeof(" + ptr_type + ")"
            stmt = row.n.stmt()
            malloc_string = ptr_type + " *" + a + " = (" + ptr_type + " *) aocl_utils::alignedMalloc(" + size + ");"
            stmt.instrument(pos='before', code='#ifdef HW\n' + malloc_string + "\n#else\n")
            stmt.instrument(pos='after', code='#endif')
            for arg in kernel_args:
                if arg['name'] == a:
                    arg['size_string'] = size
                    arg['allocated'] = True
    deletes = project.query("g:Global => d:ExprDelete")
    for row in deletes:
        if row.d.in_code():
            free_string = "aocl_utils::alignedFree(" + row.d.var().unparse().strip() + ");"            
            row.d.stmt().instrument(pos='before', code='#ifdef HW\n' + free_string + "\n#else\n")
            row.d.stmt().instrument(pos='after', code='#endif')

def declare_buffers(project, hotspot, kernel_args):
    
    calls = project.query("g:Global => c:ExprCall")
    for row in calls:
        if row.c.in_code() and row.c.name() == hotspot:
            kernel_call_params = row.c.unparse()[row.c.unparse().index('(')+1:row.c.unparse().rfind(')')].split(',')
    
    # TODO: support more than cl_int / cl_float / cl_mem 
    buffers_to_initialise = []
    buffer_params = {}
    declare_buffers_STR = "#ifdef HW\n"
    for i in range(len(kernel_args)):
        
        arg = kernel_args[i]
        
        # if "*" in arg['kernel_string']:
        if arg['is_pointer']:
            declare_buffers_STR += "static cl_mem " + arg['name'] + "_buffer = NULL;\n"
            buffer_params[arg['name']] = kernel_call_params[i]
        elif arg['type'] == "int" or arg['type'] == "const int":
            declare_buffers_STR += "static cl_int " + arg['name'] + "_buffer;\n"
            buffers_to_initialise.append({"name": arg['name'], "param":kernel_call_params[i]})
        elif arg['type'] == "float" or arg['type'] == "const float":
            declare_buffers_STR += "static cl_float " + arg['name'] + "_buffer;\n"
            buffers_to_initialise.append({"name": arg['name'], "param":kernel_call_params[i]})
        # else:
        #     declare_buffers_STR += "static cl_mem " + arg['name'] + "_buffer = NULL;\n"
        #     arg['size'] = "sizeof(%s)" % arg['type'] 
        #     arg['p_to_s'] = True 
    declare_buffers_STR += "#endif\n"

    return declare_buffers_STR, buffers_to_initialise, buffer_params

def initialise_cl_ints(buffers_to_initialise):
    initialise_cl_ints_STR = ""
    for buffer in buffers_to_initialise:
        initialise_cl_ints_STR +=  buffer["name"] + "_buffer = " + buffer["param"]  +  ";\n"
    return initialise_cl_ints_STR

def ensure_pointers_allocated(kernel_args, buffer_params):

    orig_buffer_params = {}
    allocate_args = ""
    for arg in kernel_args:
        # if 'allocated' not in arg and '*' in arg['kernel_string']:
        if 'allocated' not in arg and arg['is_pointer']:
            # ptr_type = arg['kernel_string'][:arg['kernel_string'].index('*')+1]
            # ptr_type= arg['pointer_type']
            # if arg['large']:
            #     name = arg['name']+"_"
            #     allocate_args += arg['pointer_type'] + " " + name + " = (" + ptr_type + ") aocl_utils::alignedMalloc(" + arg['size'] + ");\n"
            # else:
            name = "_"+arg['name']
            allocate_args += arg['pointer_type'] + " " + name + " = (" + arg['pointer_type'] + ") aocl_utils::alignedMalloc(" + arg['size_string'] + ");\n"
        
            if len(arg['dims']) == 1:
                allocate_args += "for (int i = 0; i < %s; i++) {\n" % str(arg['dims'][0])
                allocate_args += "    %s[i] = %s[i];\n}\n" % (name, buffer_params[arg['name']])
            else:
                if len(arg['dims']) == 2: 
                    allocate_args += "for (int i = 0; i < %s; i++) {\n" % str(arg['dims'][0])
                    allocate_args += "    for (int j = 0; j < %s; j++) {\n" % str(arg['dims'][1])
                    allocate_args += "        %s[i*%s+j] = %s[i][j];\n    }\n}\n" % (name, str(arg['dims'][1]), buffer_params[arg['name']])
            
            orig_buffer_params[arg['name']] = buffer_params[arg['name']]
            buffer_params[arg['name']] = name 
    
    return allocate_args, buffer_params, orig_buffer_params


def create_buffers(kernel_args):
    cl_rw = {'R': "CL_MEM_READ_ONLY", 'RW': "CL_MEM_READ_WRITE", 'W': "CL_MEM_WRITE_ONLY"}
    create_buffers_STR = "cl_int status1;\n"
    buffers_to_release = []
    for arg in kernel_args:
        # if "*" in arg['kernel_string'] or "p_to_s" in arg:
        if arg['is_pointer']:
            create_buffers_STR += arg['name'] + "_buffer = clCreateBuffer(context, " + cl_rw[arg['rw']] + ", " + arg['size_string'] + ", 0, &status1);\n" 
            create_buffers_STR += 'checkError(status1, "Failed to create ' + arg['name'] + ' buffer.");\n'
            buffers_to_release.append(arg['name'] + "_buffer")
    return create_buffers_STR,  buffers_to_release


def enqueue_write_buffers(kernel_args, buffer_params):
    enqueue_write_buffers_STR = "cl_int status2;\n"
    for arg in kernel_args:
        if arg['is_pointer'] and 'R' in arg['rw']:
            enqueue_write_buffers_STR += "status2 = clEnqueueWriteBuffer(queue," + arg['name'] + "_buffer,0,0," + arg['size_string'] + "," + buffer_params[arg['name']] + ",0,0,0);\n"
            enqueue_write_buffers_STR += 'checkError(status2, "Failed to enqueue write buffer ' + buffer_params[arg['name']] + '");\n'
    return enqueue_write_buffers_STR


def set_kernel_args(kernel_args):
    set_kernel_args_STR = "cl_int status3;\n"
    for i in range(0, len(kernel_args)):
        arg = kernel_args[i]
        if arg['is_pointer']:
            set_kernel_args_STR += "status3 = clSetKernelArg(hotspot_kernel," + str(i) + ",sizeof(cl_mem),&" + arg['name'] + "_buffer);\n"
        elif arg['type'] == "int" or arg['type'] == "const int":
            set_kernel_args_STR += "status3 = clSetKernelArg(hotspot_kernel," + str(i) + ",sizeof(cl_int),&" + arg['name'] + "_buffer);\n"
        elif arg['type'] == "float" or arg['type'] == "const float":
            set_kernel_args_STR += "status3 = clSetKernelArg(hotspot_kernel," + str(i) + ",sizeof(cl_float),&" + arg['name'] + "_buffer);\n"
        set_kernel_args_STR += 'checkError(status3, "Failed to set hotspot_kernel arg ' + str(i) + '");\n'
    return set_kernel_args_STR 

def enqueue_read_buffers(kernel_args, buffer_params, orig_buffer_params):

    enqueue_read_buffers_STR = "cl_int status4;\n"
    for arg in kernel_args:
        if arg['is_pointer'] and 'W' in arg['rw']:
            enqueue_read_buffers_STR += "status4 = clEnqueueReadBuffer(queue, " + arg['name'] + "_buffer, 1, 0, " + arg['size_string'] + ", " + buffer_params[arg['name']] + ", 0, 0, 0);\n"
            enqueue_read_buffers_STR += 'checkError(status4, "Failed to enqueue read buffer ' + buffer_params[arg['name']] + '");\n'
    deallocate_args_STR = ""
    for arg in kernel_args:
        if 'allocated' not in arg and arg['is_pointer']:
            # if arg['large']:
            #     name = arg['name']+"_"
            # else:
            name = "_"+arg['name']
            if 'W' in arg['rw']:
                if len(arg['dims']) == 1:
                    deallocate_args_STR += "for (int i = 0; i < %s; i++) {\n" % str(arg['dims'][0])
                    deallocate_args_STR += "    %s[i] = %s[i];\n}\n" % (orig_buffer_params[arg['name']], name)
                elif len(arg['dims']) == 2:
                    deallocate_args_STR += "for (int i = 0; i < %s; i++) {\n" % str(arg['dims'][0])
                    deallocate_args_STR += "    for (int j = 0; j < %s; j++) {\n" % str(arg['dims'][1])
                    deallocate_args_STR += "         %s[i][j] = %s[i*%s+j];\n    }\n}\n" % (orig_buffer_params[arg['name']], name, str(arg['dims'][1]))            
            deallocate_args_STR += "aocl_utils::alignedFree("+name+");\n"
    return enqueue_read_buffers_STR, deallocate_args_STR

def modify_cleanup_function(buffers_to_release):
    release_buffers = ""
    for buffer in buffers_to_release:
        release_buffers += "if(" + buffer + "){ clReleaseMemObject(" + buffer + "); }\n"

    cleanup_function = opencl_host_strings.CLEANUP.replace('*RELEASE_BUFFERS*', release_buffers)
    return cleanup_function

def populate_opencl_host(ast, path_to_source, path_to_host, kernel_args, header, hotspot, ndrange=False, gWorkSize=0):
    print("populating C++ host application... ")
    # find all calls to malloc/free and new/delete and instrument with hardware versions
    # also sets size of kernel pointer args
    project = ast.project
    instrument_dynamic_memory_allocation(project, kernel_args)
    
    ## generate all opencl string blocks and find the right places in the code to insert 
    
    # declare buffers for kernel args
    declare_buffers_STR, buffers_to_initialise, buffer_params = declare_buffers(project, hotspot, kernel_args)
    
    #  initialise cl_int/float buffers
    initialise_cl_ints_STR = initialise_cl_ints(buffers_to_initialise)

    # ensure all kernel pointer args are dynamically allocated
    allocate_args_STR, buffer_params, orig_buffer_params = ensure_pointers_allocated(kernel_args, buffer_params) 

    # createbuffers for kernel pointer args
    create_buffers_STR, buffers_to_release = create_buffers(kernel_args)

    # enqueue write buffers
    enqueue_write_buffers_STR = enqueue_write_buffers(kernel_args, buffer_params)

    # set kernel args
    set_kernel_args_STR = set_kernel_args(kernel_args)

    # enqueue read buffers, read, and deallocate
    enqueue_read_buffers_STR, deallocate_args_STR = enqueue_read_buffers(kernel_args, buffer_params, orig_buffer_params)

    # insert PROG_START and buffer declarations above main
    global_ =  project.query("g:Global")[0].g
    global_.instrument(pos='before', code='#include <sys/time.h>\n' + opencl_host_strings.PROG_START + declare_buffers_STR)

    # insert MAIN_START at beginning of main 
    main_func =  project.query("g:Global =>  f:FnDef{main}")[0].f
    main_func.body().instrument(pos='begin', code=opencl_host_strings.MAIN_START)

    before_hotspot_call  = '#ifdef HW\n'
    before_hotspot_call += initialise_cl_ints_STR
    before_hotspot_call += allocate_args_STR
    before_hotspot_call += create_buffers_STR
    before_hotspot_call += enqueue_write_buffers_STR
    before_hotspot_call += set_kernel_args_STR
    if ndrange:
        before_hotspot_call += "size_t globalWorkSize = "  + gWorkSize + ";\n"
        before_hotspot_call += opencl_host_strings.ENQUEUE_ND_KERNEL
    else:
        before_hotspot_call += opencl_host_strings.ENQUEUE_SWI_KERNEL
    before_hotspot_call += enqueue_read_buffers_STR
    before_hotspot_call += deallocate_args_STR
    before_hotspot_call += '#else\nprintf("Running on CPU ...");\nstruct timeval t1, t2;\ngettimeofday(&t1, NULL);\n'
    
    after_hotspot_call = 'gettimeofday(&t2, NULL);\n'
    after_hotspot_call += 'printf("CPU computation took %lfs", (double)(t2.tv_usec-t1.tv_usec)/1000000 + (double)(t2.tv_sec-t1.tv_sec));\n'
    after_hotspot_call += '#endif\n'

    # instrument code with string blocks before and after hotspot call 
    calls = project.query("g:Global => c:ExprCall")
    hotspot_call = [row for row in calls if row.c.in_code() and row.c.name() == hotspot][0]
    hotspot_call.c.stmt().instrument(pos='before', code=before_hotspot_call)
    hotspot_call.c.stmt().instrument(pos='after', code=after_hotspot_call)

    # modify cleanup function to release created buffers, call before end of main   
    cleanup_function = modify_cleanup_function(buffers_to_release)
    main_func.instrument(pos='after', code=cleanup_function)
    instrument_block(main_func.body(), "#ifdef  HW\ncleanup();\n#endif", exit=True)


    ## TODO: fix below issues with parsing
    
    #remove 'const' in function declarations (pointer issues)
    func_defs = project.query('g:Global => f:FnDef')
    for row in func_defs:
        if row.f.in_code():
            row.f.decl().instrument(pos="replace", code=row.f.decl().unparse().replace("const", "")) 
    ast.commit()

    for f in os.listdir(path_to_source):
        if f.endswith(".cpp"):
            subprocess.call(['cp', path_to_source + "/" + f, path_to_host])
            
            #BUG: remove ::__cxx11::
            with open(path_to_host + str(f).strip(), 'r') as f1:
                contents = f1.read()
            contents = contents.replace("std::__cxx11::", "std::")
            contents = contents.replace("std::sqrt", "sqrt")
            #BUG: remove const
            contents = contents.replace("const", "")
            with open(path_to_host + str(f).strip(), 'w') as f1:
                f1.write(contents)
