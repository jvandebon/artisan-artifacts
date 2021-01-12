#ifndef __HLS_STDIO_H__
#define __HLS_STDIO_H__

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifdef HLS_SYNTHESIS
// Suppress if used in component
# define printf(...) 

#endif //HLS_SYNTHESIS
#endif //__HLS_STDIO_H__
