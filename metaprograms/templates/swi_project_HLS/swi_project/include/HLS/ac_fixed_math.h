/**************************************************************************
 *                                                                        *
 *  Math Library Function Wrapper for ac_fixed datatype                   *
 *  Copyright 2017, Intel Corporation                                     *
 *  All Rights Reserved.                                                  *
 *                                                                        *
 *  Author: Domi Yan                                                      *
 *                                                                        *
 **************************************************************************
 * 
 ************** Function Support and Input Limit *************** 
 * 
 * Notice: W, I, S here stands for value in (input value) ac_fixed<W, I, S> template
 *
 * Function Name              Input Limit    
 * sqrt_fixed                 W <= 32, undefined behaviour for input value < 0
 * reciprocal_fixed           W <= 32
 * reciprocal_sqrt_fixed      W <= 32, undefined behaviour for input value < 0
 * sin_fixed                  For signed (S == true), I == 3, W <= 64, undefined behaviour for value outside of [-pi, pi];
 *                            For unsigned (S == false), I == 1, W <= 64, undefined behaviour for value outside of [0, pi/2]
 * cos_fixed                  For signed (S == true), I == 3, W <= 64, undefined behaviour for value outside of [-pi, pi];
 *                            For unsigned (S == false), I == 1, W <= 64, undefined behaviour for value outside of [0, pi/2]
 * sincos_fixed               For signed (S == true), I == 3, W <= 64, undefined behaviour for value outside of [-pi, pi];
 *                            For unsigned (S == false), I == 1, W <= 64, undefined behaviour for value outside of [0, pi/2]
 * sinpi_fixed                I >= 0, W <= 32
 * cospi_fixed                I >= 0, W <= 32
 * sincospi_fixed             I >= 0, W <= 32
 * log_fixed                  S == true, I >= 2, W - I >= 2 (2 fraction bits at least)
 * exp_fixed                  S == true, I >= 2, W - I >= 2 (2 fraction bits at least)
 *
 ******** Output (Return Value) Type Propagation Rule  ************** 
 *
 * Notice: W, I, S here stands for value in (input value) ac_fixed<W, I, S> template
 *         rW, rI, rS here stands for value in (return value) ac_fixed<rW, rI, rS> template
 *
 * Function Name              Type Propagation Rule
 * sqrt_fixed                 rI = I, rW = W, rS = S
 * reciprocal_fixed           rI = I, rW = W, rS = S
 * reciprocal_sqrt_fixed      W <= 32, undefined behaviour for input value < 0
 * sin_fixed                  For signed (S == true), rI == 2, rW =  W - I + 2; 
 *                            For unsigned (S == false), I == 1, rW =  W - I + 1
 * cos_fixed                  For signed (S == true), rI == 2, rW =  W - I + 2; 
 *                            For unsigned (S == false), I == 1, rW =  W - I + 1
 * sincos_fixed               For signed (S == true), rI == 2, rW =  W - I + 2; 
 *                            For unsigned (S == false), I == 1, rW =  W - I + 1
 * sinpi_fixed                rI = 2, rW = W - I + 2, rS = true
 * cospi_fixed                rI = 2, rW = W - I + 2, rS = true
 * sincospi_fixed             rI = 2, rW = W - I + 2, rS = true
 * log_fixed                  rI = I, rW = W, rS = S
 * exp_fixed                  rI = I, rW = W, rS = S
 *
 ******************************************************************  
*/

#ifndef __HLS_AC_FIXED_MATH_H__
#define __HLS_AC_FIXED_MATH_H__

#include "HLS/ac_int.h"
#include "HLS/ac_fixed.h"
#include "HLS/ac_fixed_math_internal.h"

template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::sqrt_r sqrt_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::rcpl_r reciprocal_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::rcpl_sqrt_r reciprocal_sqrt_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::sin_r sin_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::cos_r cos_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> void sincos_fixed(ac_fixed<W, I, S> x, typename ac_fixed_math_private::rt<W, I, S>::sin_r & sin_ret, 
                                                                      typename ac_fixed_math_private::rt<W, I, S>::cos_r & cos_ret);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::sinpi_r sinpi_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::cospi_r cospi_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> void sincospi_fixed(ac_fixed<W, I, S> x, typename ac_fixed_math_private::rt<W, I, S>::sinpi_r & sinpi_ret, 
                                                                        typename ac_fixed_math_private::rt<W, I, S>::cospi_r & cospi_ret );
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::exp_r exp_fixed(ac_fixed<W, I, S> x);
template<int W, int I, bool S> typename ac_fixed_math_private::rt<W, I, S>::log_r log_fixed(ac_fixed<W, I, S> x);


template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::sqrt_r
sqrt_fixed(ac_fixed<W, I, S> x){
  static_assert(W <= 32, "");
  typename ac_fixed_math_private::rt<W, I, S>::sqrt_r ret;
  ac_private::ap_int<W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_sqrt_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_sqrt__(S, &op, W, I, &r, W, I);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::rcpl_r
reciprocal_fixed(ac_fixed<W, I, S> x){
  static_assert(W <= 32, "");
  typename ac_fixed_math_private::rt<W, I, S>::rcpl_r ret;
  ac_private::ap_int<W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_reciprocal_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_reciprocal__(S, &op, W, I, &r, W, I);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::rcpl_sqrt_r
reciprocal_sqrt_fixed(ac_fixed<W, I, S> x){
  static_assert(W <= 32, "");
  typename ac_fixed_math_private::rt<W, I, S>::rcpl_sqrt_r ret;
  ac_private::ap_int<W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_reciprocal_sqrt_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_reciprocal_sqrt__(S, &op, W, I, &r, W, I);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::sin_r
sin_fixed(ac_fixed<W, I, S> x){
  static_assert((S && I == 3) || ( (!S) && I == 1) , "");
  static_assert(W <= 64, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::sin_w;
  typename ac_fixed_math_private::rt<W, I, S>::sin_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_sin_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_sin__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::sin_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::cos_r
cos_fixed(ac_fixed<W, I, S> x){
  static_assert((S && I == 3) || ( (!S) && I == 1) , "");
  static_assert(W <= 64, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::cos_w;
  typename ac_fixed_math_private::rt<W, I, S>::cos_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_cos_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_cos__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::cos_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
void
sincos_fixed(ac_fixed<W, I, S> x,
  typename ac_fixed_math_private::rt<W, I, S>::sin_r & sin_ret,
  typename ac_fixed_math_private::rt<W, I, S>::cos_r & cos_ret){
  static_assert((S && I == 3) || ( (!S) && I == 1), "");
  static_assert(W <= 64, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::cos_w;
  ac_private::ap_int<RET_W> r_cos;
  ac_private::ap_int<RET_W> r_sin;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_rs, l_rc;
  __ac_fixed_sincos_x86(W, I, S, l_op, l_rs, l_rc);
  r_cos = l_rc;
  r_sin = l_rs;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_sincos__(S, &op, W, I, &r_sin, &r_cos, RET_W, ac_fixed_math_private::rt<W, I, S>::sin_i);
#endif
  cos_ret.set_op_signed(r_cos);
  sin_ret.set_op_signed(r_sin);
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::sinpi_r
sinpi_fixed(ac_fixed<W, I, S> x){
  static_assert(I >= 0, "");
  static_assert(W - I >= 4, "");
  static_assert(W <= 32, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::sinpi_w;
  typename ac_fixed_math_private::rt<W, I, S>::sinpi_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_sinpi_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_sinpi__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::sinpi_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::cospi_r
cospi_fixed(ac_fixed<W, I, S> x){
  static_assert(I >= 0, "");
  static_assert(W - I >= 4, "");
  static_assert(W <= 32, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::cospi_w;
  typename ac_fixed_math_private::rt<W, I, S>::cospi_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_cospi_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_cospi__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::cospi_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
void
sincospi_fixed(ac_fixed<W, I, S> x, 
  typename ac_fixed_math_private::rt<W, I, S>::sinpi_r & sinpi_ret, 
  typename ac_fixed_math_private::rt<W, I, S>::cospi_r & cospi_ret ){
  static_assert(I >= 0, "");
  static_assert(W - I >= 4, "");
  static_assert(W <= 32, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::cospi_w;
  ac_private::ap_int<RET_W> r_cos;
  ac_private::ap_int<RET_W> r_sin;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_rs, l_rc;
  __ac_fixed_sincospi_x86(W, I, S, l_op, l_rs, l_rc);
  r_cos = l_rc;
  r_sin = l_rs;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_sincospi__(S, &op, W, I, &r_sin, &r_cos, RET_W, ac_fixed_math_private::rt<W, I, S>::sinpi_i);
#endif
  cospi_ret.set_op_signed(r_cos);
  sinpi_ret.set_op_signed(r_sin);
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::exp_r
exp_fixed(ac_fixed<W, I, S> x){
  static_assert(S, "");
  static_assert(I >= 2, "");
  static_assert(W - I >= 2, "");
  static_assert(W <= 32, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::exp_w;
  typename ac_fixed_math_private::rt<W, I, S>::exp_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_exp_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_exp__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::exp_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

template<int W, int I, bool S>
typename ac_fixed_math_private::rt<W, I, S>::log_r
log_fixed(ac_fixed<W, I, S> x){
  static_assert(S, "");
  static_assert(I >= 2, "");
  static_assert(W - I >= 2, "");
  static_assert(W <= 32, "");
  const int RET_W = ac_fixed_math_private::rt<W, I, S>::log_w;
  typename ac_fixed_math_private::rt<W, I, S>::log_r ret;
  ac_private::ap_int<RET_W> r;
#ifdef HLS_X86
  long long l_op = S? (long long)(x.get_op_signed()) : (unsigned long long)(x.get_op_unsigned());
  long long l_r;
  __ac_fixed_log_x86(W, I, S, l_op, l_r);
  r = l_r;
#else
  ac_private::ap_int<W> op = x.get_op_signed();
  __acl_ac_fixed_log__(S, &op, W, I, &r, RET_W, ac_fixed_math_private::rt<W, I, S>::log_i);
#endif
  ret.set_op_signed(r);
  return ret;
}

#endif //__HLS_AC_FIXED_MATH_H__
