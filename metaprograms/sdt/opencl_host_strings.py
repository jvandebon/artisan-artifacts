PROG_START = '#ifdef HW\n'
PROG_START += '    #include "CL/opencl.h"\n'
PROG_START += '    #include "AOCLUtils/aocl_utils.h"\n'
PROG_START += '    using namespace aocl_utils;\n'
PROG_START += '    static cl_platform_id platform = NULL;\n'
PROG_START += '    static cl_device_id device = NULL;\n'
PROG_START += '    static cl_context context = NULL;\n'
PROG_START += '    static cl_command_queue queue = NULL;\n'
PROG_START += '    static cl_kernel kernel_lib = NULL;\n'
PROG_START += '    static cl_program program = NULL;\n'
PROG_START += '    void cleanup();\n'
PROG_START += '#endif\n'

MAIN_START = '#ifdef HW\n'
MAIN_START += '    cl_int status;\n'
MAIN_START += '    if(!setCwdToExeDir()) {\n'
MAIN_START += '        return false;\n'
MAIN_START += '    }\n'
MAIN_START += '    platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");\n'
MAIN_START += '    if(platform == NULL) {\n'
MAIN_START += '    	printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.");\n'
MAIN_START += '    	return false;\n'
MAIN_START += '    }\n'
MAIN_START += '    scoped_array<cl_device_id> devices;\n'
MAIN_START += '    cl_uint num_devices;\n'
MAIN_START += '    devices.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));\n'
MAIN_START += '    device = devices[0];\n'
MAIN_START += '    context = clCreateContext(NULL, 1, &device, &oclContextCallback, NULL, &status);\n'
MAIN_START += '    checkError(status, "Failed to create context");\n'
MAIN_START += '    queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);\n'
MAIN_START += '    checkError(status, "Failed to create command queue");\n'
MAIN_START += '    std::string binary_file = getBoardBinaryFile("kernel", device);\n'
MAIN_START += '    printf("Using AOCX: %s", binary_file.c_str());\n'
MAIN_START += '    program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);\n'
MAIN_START += '    status = clBuildProgram(program, 0, NULL, "", NULL, NULL);\n'
MAIN_START += '    checkError(status, "Failed to build program");\nkernel_lib = clCreateKernel(program, "kernel_lib", &status);\n'
MAIN_START += '    checkError(status, "Failed to create kernel");\n'
MAIN_START += '#endif\n'

ENQUEUE_KERNEL = 'printf("Enqueueing FPGA kernel...");\n'
ENQUEUE_KERNEL += 'cl_int status0;\n'
ENQUEUE_KERNEL += 'double lib_start = aocl_utils::getCurrentTimestamp();\n'
ENQUEUE_KERNEL += 'status0 = clEnqueueTask(queue, kernel_lib, 0, NULL, NULL);\n'
ENQUEUE_KERNEL += 'checkError(status0, "Failed to launch kernel_lib");\n'
ENQUEUE_KERNEL += 'status0 = clFinish(queue);\n'
ENQUEUE_KERNEL += 'checkError(status0, "Failed to finish");\n'
ENQUEUE_KERNEL += 'double lib_stop = aocl_utils::getCurrentTimestamp();\n'
ENQUEUE_KERNEL += 'printf ("Kernel computation took %g seconds", lib_stop - lib_start);\n'

CLEANUP = '#ifdef HW\n'
CLEANUP += '    void cleanup(){\n'
CLEANUP += '        if(kernel_lib) { clReleaseKernel(kernel_lib); }\n'
CLEANUP += '        if(program) { clReleaseProgram(program); }\n'
CLEANUP += '        if(queue) { clReleaseCommandQueue(queue); }\n'
CLEANUP += '        if(context) { clReleaseContext(context); }\n'
CLEANUP += '        *RELEASE_BUFFERS*'
CLEANUP += '    }\n'
CLEANUP += '#endif\n'



