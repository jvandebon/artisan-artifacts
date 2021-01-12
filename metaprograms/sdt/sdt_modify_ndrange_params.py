#!/usr/bin/env artisan
from artisan.core import *
from artisan.rose import *

def modify_ndrange_params(kernel_path, workgroupsize, numsimditems):

    new_file_str = ""

    with open(kernel_path, 'r') as f:   
        for line in f:
            if '#define SIMD' in line:
                new_file_str += "#define SIMD " + str(numsimditems) +"\n"
            elif '#define WG' in line:
                new_file_str += "#define WG " + str(workgroupsize) +"\n"
            else:
                new_file_str += line
        
    with open(kernel_path, 'w') as f:
        f.write(new_file_str)

# modify_ndrange_params('./ndrange_project/project/device/kernel.cl', 16, 8)



