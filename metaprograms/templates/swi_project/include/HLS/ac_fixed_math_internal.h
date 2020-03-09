#ifndef __HLS_AC_FIXED_MATH_INTERNAL_H__
#define __HLS_AC_FIXED_MATH_INTERNAL_H__

#include "HLS/ac_fixed.h"

// Declartion of X86 emulation flow signitures
extern "C" void __ac_fixed_sqrt_x86(int w, int i, int s, long long x, long long&r);
extern "C" void __ac_fixed_reciprocal_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_reciprocal_sqrt_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_sin_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_cos_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_sinpi_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_cospi_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_exp_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_log_x86(int w, int i, int s, long long x, long long &r);
extern "C" void __ac_fixed_sincos_x86(int w, int i, int s, long long x, long long &rs, long long &rc);
extern "C" void __ac_fixed_sincospi_x86(int w, int i, int s, long long x, long long &rs, long long &rc);

// Declartion of FPGA flow signitures
extern "C" void __acl_ac_fixed_sqrt__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_reciprocal__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_reciprocal_sqrt__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_sin__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_cos__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_sincos__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r_sin, void * ptr_r_cos, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_sinpi__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_cospi__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_sincospi__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r_sin, void * ptr_r_cos, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_exp__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);
extern "C" void __acl_ac_fixed_log__(bool sign, void * ptr_op, int w_op, int i_op, void * ptr_r, int w_ret, int i_ret);

namespace ac_fixed_math_private{
  // Check Here for Type propatation
  template<int W, int I, bool S>
  struct rt {
    enum {
      //sqrt
      sqrt_w = W,
      sqrt_i = I,
      sqrt_s = S,
      //reciprocal
      rcpl_w = W,
      rcpl_i = I,
      rcpl_s = S,
      //reciprocal_sqrt
      rcpl_sqrt_w = W,
      rcpl_sqrt_i = I,
      rcpl_sqrt_s = S,
      //helper
      i_sincos = S? 2 : 1,
      w_sincos = W - I + i_sincos,
      //sin
      sin_w = w_sincos,
      sin_i = i_sincos,
      sin_s = S,
      //cos
      cos_w = w_sincos,
      cos_i = i_sincos,
      cos_s = S,
      //sinpi
      sinpi_w = W - I + 2,
      sinpi_i = 2,
      sinpi_s = true,
      //cospi
      cospi_w = sinpi_w,
      cospi_i = sinpi_i,
      cospi_s = sinpi_s,
      //exp
      exp_w = W,
      exp_i = I,
      exp_s = true,
      //log
      log_w = W,
      log_i = I,
      log_s = true,
    };
   // typedef ac_fixed<div_w, div_i, div_s> div;
    typedef ac_fixed<W, I, S> input_t;
    typedef ac_fixed<sqrt_w, sqrt_i, sqrt_s> sqrt_r;
    typedef ac_fixed<rcpl_w, rcpl_i, rcpl_s> rcpl_r;
    typedef ac_fixed<rcpl_sqrt_w, rcpl_sqrt_i, rcpl_sqrt_s> rcpl_sqrt_r;
    typedef ac_fixed<sin_w, sin_i, sin_s> sin_r;
    typedef ac_fixed<cos_w, cos_i, cos_s> cos_r;
    typedef ac_fixed<sinpi_w, sinpi_i, sinpi_s> sinpi_r;
    typedef ac_fixed<cospi_w, cospi_i, cospi_s> cospi_r;
    typedef ac_fixed<exp_w, exp_i, exp_s> exp_r;
    typedef ac_fixed<log_w, log_i, log_s> log_r;
  };
};

#endif //__HLS_AC_FIXED_MATH_INTERNAL_H__