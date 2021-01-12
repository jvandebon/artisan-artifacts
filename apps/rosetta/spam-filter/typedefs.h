/*===============================================================*/
/*                                                               */
/*                          typedefs.h                           */
/*                                                               */
/*              Constant definitions and typedefs.               */
/*                                                               */
/*===============================================================*/

#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__



// dataset information
#define NUM_FEATURES   1024
#define NUM_SAMPLES    5000
#define NUM_TRAINING   4500
#define NUM_TESTING    500
#define STEP_SIZE      60000 
#define NUM_EPOCHS     5
#define DATA_SET_SIZE  NUM_FEATURES*NUM_SAMPLES


// software version uses C++ built-in datatypes
typedef float FeatureType;
typedef float DataType;
typedef unsigned char LabelType;
// and uses math functions to compute sigmoid values
// no need to declare special datatype for sigmoid

#define PAR_FACTOR 32

#endif
