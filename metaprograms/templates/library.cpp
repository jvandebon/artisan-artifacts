#include "HLS/math.h"
#include <cstdint>

#define OCL_ADDRSP_CONSTANT const __attribute__((address_space(2)))
#define OCL_ADDRSP_GLOBAL __attribute__((address_space(1)))
#define OCL_ADDRSP_PRIVATE __attribute__((address_space(0)))
#define OCL_ADDRSP_LOCAL __attribute__((address_space(3)))

*OTHER_FUNCS*

extern "C"{
void lib_func(*FUNC_ARGS*)  
*FUNC_BODY*
} 
