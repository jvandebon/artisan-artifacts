/*===============================================================*/
/*                                                               */
/*                          typedefs.h                           */
/*                                                               */
/*           Constant definitions and typedefs for host.         */
/*                                                               */
/*===============================================================*/

#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

// dataset information
#define NUM_TRAINING 18000
#define CLASS_SIZE 1800
#define NUM_TEST 2000
#define DIGIT_WIDTH 4

// typedefs
typedef unsigned long long DigitType;
typedef unsigned char      LabelType;

// parameters
#define K_CONST 3
#define PAR_FACTOR 40


// types and constants used in the functions below
const unsigned long long m1  = 0x5555555555555555; //binary: 0101...
const unsigned long long m2  = 0x3333333333333333; //binary: 00110011..
const unsigned long long m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...


#endif
