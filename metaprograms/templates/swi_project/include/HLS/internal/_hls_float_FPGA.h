#ifndef _INTEL_IHC_HLS_INTERNAL_HLS_FLOAT_FPGA
#define _INTEL_IHC_HLS_INTERNAL_HLS_FLOAT_FPGA

#include "_hls_float_common_inc.h"

namespace ihc {

namespace internal {

using RD_t = fp_config::FP_Round;

///////////////////////////// Binary Operators ///////////////////////////////

template <int AccuracyLevel, int SubnormalSupport, int Eout, int Mout, RD_t Rndout,
          int E1,int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
inline void hls_vpfp_add(hls_float<Eout, Mout, Rndout> &ret,
                         const hls_float<E1, M1, Rnd1> &arg1,
                         const hls_float<E2, M2, Rnd2> &arg2) {

    ac_int<Eout + Mout + 1, false> temp;
    temp._set_value_internal(__builtin_intel_hls_vpfp_add(
                                        arg1._get_bits_ap_uint(), E1, M1, 
                                        arg2._get_bits_ap_uint(), E2, M2,
                                        AccuracyLevel, SubnormalSupport, 
                                        Eout, Mout));
    ret.set_bits(temp);
}

template <int AccuracyLevel, int SubnormalSupport, int Eout, int Mout, RD_t Rndout,
          int E1,int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
inline void hls_vpfp_sub(hls_float<Eout, Mout, Rndout> &ret,
                         const hls_float<E1, M1, Rnd1> &arg1,
                         const hls_float<E2, M2, Rnd2> &arg2) {

    ac_int<Eout + Mout + 1, false> temp;
    temp._set_value_internal(__builtin_intel_hls_vpfp_sub(
                                        arg1._get_bits_ap_uint(), E1, M1, 
                                        arg2._get_bits_ap_uint(), E2, M2,
                                        AccuracyLevel, SubnormalSupport, 
                                        Eout, Mout));
    ret.set_bits(temp);

}

template <int AccuracyLevel, int SubnormalSupport, int Eout, int Mout, RD_t Rndout,
          int E1,int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
inline void hls_vpfp_mul(hls_float<Eout, Mout, Rndout> &ret,
                         const hls_float<E1, M1, Rnd1> &arg1,
                         const hls_float<E2, M2, Rnd2> &arg2) {

    ac_int<Eout + Mout + 1, false> temp;
    temp._set_value_internal(__builtin_intel_hls_vpfp_mul(
                                        arg1._get_bits_ap_uint(), E1, M1, 
                                        arg2._get_bits_ap_uint(), E2, M2,
                                        AccuracyLevel, SubnormalSupport, 
                                        Eout, Mout));
    ret.set_bits(temp);

}

template <int AccuracyLevel, int SubnormalSupport, int Eout, int Mout, RD_t Rndout,
          int E1,int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
inline void hls_vpfp_div(hls_float<Eout, Mout, Rndout> &ret,
                         const hls_float<E1, M1, Rnd1> &arg1,
                         const hls_float<E2, M2, Rnd2> &arg2) {

    ac_int<Eout + Mout + 1, false> temp;
    temp._set_value_internal(__builtin_intel_hls_vpfp_div(
                                        arg1._get_bits_ap_uint(), E1, M1, 
                                        arg2._get_bits_ap_uint(), E2, M2,
                                        AccuracyLevel, SubnormalSupport, 
                                        Eout, Mout));
    ret.set_bits(temp);

}

//////////////////////////// Conversions /////////////////////////////////

// between hls_float
template <int Ein, int Min, int Eout, int Mout, RD_t Rin, RD_t Rout>
inline void hls_vpfp_cast(const hls_float<Ein, Min, Rin> &from,
                          hls_float<Eout, Mout, Rout> &to) {
  
  if (Ein == Eout && Min == Mout){
    to.set_bits(from.get_bits());
  }
  // 4 == RoundtoZero in hls_float.h
  else if (Rout == 4) {
    // We decide to implement RoundToZero in source because
    // the logic is fairly simple and can be easily const-propagated
    hls_vpfp_trunc_convert(from, to);
  }
  else{
    // for the other rounding blocks we rely on dsdk::A_CAST_BLOCK to handle it
    // for us. This is because the logic is slightly more complicated, the dsdk implementation is
    // more robustly tested and optimized compared to source level implementation.

    ac_int<Eout + Mout + 1, false> temp;
    temp._set_value_internal(__builtin_intel_hls_vpfp_cast(
                                        from._get_bits_ap_uint(), Ein, Min, 
                                        Rout, Eout, Mout));
    to.set_bits(temp);
  }
}

// from integer type to hls_float conversions
template <typename T, int Eout, int Mout, RD_t Rout>
inline void hls_vpfp_cast_integral(const T &from, hls_float<Eout, Mout, Rout> &to) {
  static_assert(std::is_integral<T>::value, "this function only supports casting from integer types");
  // cast the input integer to an ac_int
  ac_int<sizeof(T)*8, std::is_signed<T>::value> from_cast(from);

  ac_int<Eout + Mout + 1, false> temp;
  // built-ins are *very* strong typed. Each argument must be explicitly an integer type
  temp._set_value_internal(__builtin_intel_hls_vpfp_cast_from_fixed(
                                      from_cast._get_value_internal(), 
                                      (int)sizeof(T)*8, 0, (int)std::is_signed<T>::value,
                                      Eout, Mout));
  to.set_bits(temp);
}

// from hls_float to integral type conversions
template <typename T, int Eout, int Mout, RD_t Rout>
inline void hls_vpfp_cast_integral(const hls_float<Eout, Mout, Rout> &from, T &to) {
  static_assert(std::is_integral<T>::value, "this function only supports casting to integer types");
  // construct an equivalent ac_int representation of the same integer
  ac_int<sizeof(T)*8, std::is_signed<T>::value> to_cast;
  to_cast._set_value_internal(__builtin_intel_hls_vpfp_cast_to_fixed(
                                      from._get_bits_ap_uint(), Eout, Mout,
                                      (int)sizeof(T)*8, 0, (int)std::is_signed<T>::value));
  //cast this output to the integer of type T
  to = (T)to_cast;
}



/////////////////////////// Relational Operator //////////////////////////////

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2,
          int AccuracyLevel = 0, int SubnormalSupport = 0>
inline bool hls_vpfp_gt(const hls_float<E1, M1, Rnd1> &arg1,
                        const hls_float<E2, M2, Rnd2> &arg2) {

  return __builtin_intel_hls_vpfp_gt(arg1._get_bits_ap_uint(), E1, M1, arg2._get_bits_ap_uint(), E2, M2);

}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2,
          int AccuracyLevel = 0, int SubnormalSupport = 0>
inline bool hls_vpfp_lt(const hls_float<E1, M1, Rnd1> &arg1,
                        const hls_float<E2, M2, Rnd2> &arg2) {

  return __builtin_intel_hls_vpfp_lt(arg1._get_bits_ap_uint(), E1, M1, arg2._get_bits_ap_uint(), E2, M2);

}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2,
          int AccuracyLevel = 0, int SubnormalSupport = 0>
inline bool hls_vpfp_eq(const hls_float<E1, M1, Rnd1> &arg1,
                        const hls_float<E2, M2, Rnd2> &arg2) {

  return __builtin_intel_hls_vpfp_eq(arg1._get_bits_ap_uint(), E1, M1, arg2._get_bits_ap_uint(), E2, M2);

}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2,
          int AccuracyLevel = 0, int SubnormalSupport = 0>
inline bool hls_vpfp_ge(const hls_float<E1, M1, Rnd1> &arg1,
                        const hls_float<E2, M2, Rnd2> &arg2) {

  return __builtin_intel_hls_vpfp_ge(arg1._get_bits_ap_uint(), E1, M1, arg2._get_bits_ap_uint(), E2, M2);

}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2,
          int AccuracyLevel = 0, int SubnormalSupport = 0>
inline bool hls_vpfp_le(const hls_float<E1, M1, Rnd1> &arg1,
                        const hls_float<E2, M2, Rnd2> &arg2) {

  return __builtin_intel_hls_vpfp_le(arg1._get_bits_ap_uint(), E1, M1, arg2._get_bits_ap_uint(), E2, M2);

}



/////////////////////// Commonly Used Math Operations ////////////////////////

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> sqrt_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_sqrt(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> cbrt_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_cbrt(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> recip_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_recip(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> rsqrt_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_rsqrt(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> hypot_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                  hls_float<E2, M2, Rnd2> const &y) {
  hls_float<E1, M1> ret;
  ac_int<E1 + M1 + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_hypot(
                                                x._get_bits_ap_uint(), E1, M1, 
                                                y._get_bits_ap_uint(), E2, M2,
                                                E1, M1));
  ret.set_bits(temp);
  return ret;
}

////////////////// Exponential and Logarithmic Functions /////////////////////

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> exp_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_exp(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> exp2_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_exp2(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> exp10_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_exp10(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> expm1_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_expm1(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> log_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_log(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> log2_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_log2(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> log10_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_log10(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> log1p_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_log1p(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

///////////////////////// Power Functions ////////////////////////////////

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> pow_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                hls_float<E2, M2, Rnd2> const &y) {
  hls_float<E1, M1> ret;
  ac_int<E1 + M1 + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_pow(
                                      x._get_bits_ap_uint(), E1, M1,
                                      y._get_bits_ap_uint(), E2, M2,
                                      E1, M1));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd, int W, bool S>
hls_float<E, M, Rnd> pown_vpfp_impl(hls_float<E, M, Rnd> const &x, 
                                   ac_int<W, S>    const &n) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_pown(
                                      n._get_value_internal(),  W, 0, (int)S,
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> powr_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                hls_float<E2, M2, Rnd2> const &y) {
  hls_float<E1, M1> ret;
  ac_int<E1 + M1 + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_powr(
                                      x._get_bits_ap_uint(), E1, M1,
                                      y._get_bits_ap_uint(), E2, M2,
                                      E1, M1));
  ret.set_bits(temp);
  return ret;
}

/////////////////////// Trigonometric Functions /////////////////////////////


template <int E, int M, RD_t Rnd>
inline hls_float<E, M, Rnd> sin_vpfp_impl(const hls_float<E, M, Rnd> &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_sin(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> sinpi_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_sinpi(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> cos_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_cos(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> cospi_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_cospi(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> sincos_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                   hls_float<E2, M2, Rnd2> &cos_value) {
  hls_float<E1, M1> ret;
  const int FPWidth = E1+M1+1;
  // contains both values
  ac_int<FPWidth + FPWidth, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_sincos(
                                      x._get_bits_ap_uint(), E1, M1,
                                      E1, M1, E2, M2));
  ret.set_bits(temp.template slc<FPWidth>(0));
  cos_value.set_bits(temp.template slc<FPWidth>(FPWidth));
  return ret;
}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> sincospi_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                   hls_float<E2, M2, Rnd2> &cos_value) {
  hls_float<E1, M1> ret;
  const int FPWidth = E1+M1+1;
  // contains both values
  ac_int<FPWidth + FPWidth, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_sincospi(
                                      x._get_bits_ap_uint(), E1, M1,
                                      E1, M1, E2, M2));
  ret.set_bits(temp.template slc<FPWidth>(0));
  cos_value.set_bits(temp.template slc<FPWidth>(FPWidth));
  return ret;
}


template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> asin_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_asin(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> asinpi_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_asinpi(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> acos_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_acos(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> acospi_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_acospi(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> atan_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_atan(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E, int M, RD_t Rnd> 
hls_float<E, M, Rnd> atanpi_vpfp_impl(hls_float<E, M, Rnd> const &x) {
  hls_float<E, M, Rnd> ret;
  ac_int<E + M + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_atanpi(
                                      x._get_bits_ap_uint(), E, M,
                                      E, M));
  ret.set_bits(temp);
  return ret;
}

template <int E1, int M1, int E2, int M2, RD_t Rnd1, RD_t Rnd2>
hls_float<E1, M1> atan2_vpfp_impl(hls_float<E1, M1, Rnd1> const &x,
                                  hls_float<E2, M2, Rnd2> const &y) {
  hls_float<E1, M1> ret;
  ac_int<E1 + M1 + 1, false> temp;
  temp._set_value_internal(__builtin_intel_hls_vpfp_atan2(
                                      x._get_bits_ap_uint(), E1, M1,
                                      y._get_bits_ap_uint(), E2, M2,
                                      E1, M1));
  ret.set_bits(temp);
  return ret;
}

} // namespace internal
} //  namespace ihc
#endif // _INTEL_IHC_HLS_INTERNAL_HLS_FLOAT_FPGA_
