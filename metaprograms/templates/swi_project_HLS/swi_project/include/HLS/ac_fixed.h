/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 3.7                                                 *
 *                                                                        *
 *  Release Date    : Wed Jun  1 13:21:52 PDT 2016                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 3.7.0                                               *
 *                                                                        *
 *  Copyright 2005-2016, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *  
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  This file was modified by the Intel High Level Design team to         *
 *  generate efficient hardware for the Intel High Level Synthesis        *
 *  compiler. The API remains the same as defined by Mentor Graphics      *
 *  in the documentation for ac_fixed.h                                     *
 *                                                                        *
 *************************************************************************/
/*
//  Source:         ac_fixed.h
//  Description:    class for fixed point operation handling in C++
//  Author:         Andres Takach, Ph.D.
//  Modified by:    Domi Yan
//
//  Notes:
//   - Compiler support: Works with the Intel i++ compiler. For g++, use the
//                       original ac_int.h header file.
*/

#ifndef __ALTR_AC_FIXED_H
#define __ALTR_AC_FIXED_H

#include "HLS/ac_int.h"

#define __HLS_AC_FRAC_BITS_SINGLE 23
#define __HLS_AC_EXP_BITS_SINGLE 8
#define __HLS_AC_FRAC_BITS_DOUBLE 52
#define __HLS_AC_EXP_BITS_DOUBLE 11

#define __HLS_AC_F_SINGLE  ((1 << (__HLS_AC_EXP_BITS_SINGLE - 1)) - 1 + __HLS_AC_FRAC_BITS_SINGLE ) // 150
#define __HLS_AC_F_DOUBLE  ((1 << (__HLS_AC_EXP_BITS_DOUBLE - 1)) - 1 + __HLS_AC_FRAC_BITS_DOUBLE ) // 1075

#define __HLS_AC_W_SINGLE ( (1 << __HLS_AC_EXP_BITS_SINGLE) - 1 + __HLS_AC_FRAC_BITS_SINGLE + 1 + 1) // 280
#define __HLS_AC_W_DOUBLE ( (1 << __HLS_AC_EXP_BITS_DOUBLE) - 1 + __HLS_AC_FRAC_BITS_DOUBLE + 1 + 1) // 2101

#define __HLS_AC_I_SINGLE  (__HLS_AC_W_SINGLE - __HLS_AC_F_SINGLE) // 130
#define __HLS_AC_I_DOUBLE  (__HLS_AC_W_DOUBLE - __HLS_AC_F_DOUBLE) // 1026

typedef ac_fixed<__HLS_AC_W_SINGLE, __HLS_AC_I_SINGLE, true, AC_RND, AC_SAT> __hls_ac_ret_single; //ac_fixed<280, 130, true>
typedef ac_fixed<__HLS_AC_W_DOUBLE, __HLS_AC_I_DOUBLE, true, AC_RND, AC_SAT> __hls_ac_ret_double; //ac_fixed<2101, 1026, true>

#if (defined(__GNUC__) && __GNUC__ < 3 && !defined(__EDG__))
#error GCC version 3 or greater is required to include this header file
#endif

#if (defined(_MSC_VER) && _MSC_VER < 1400 && !defined(__EDG__))
#error Microsoft Visual Studio 8 or newer is required to include this header file
#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

namespace ac_private {
  template<typename T>
  struct rt_ac_fixed_T {
    template<int W, int I, bool S>
    struct op1 {
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::mult mult;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::plus plus;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::minus2 minus;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::minus minus2;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::logic logic;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::div2 div;
      typedef typename T::template rt_T< ac_fixed<W,I,S,AC_TRN,AC_WRAP> >::div div2;
    };
  };
  
  // specializations after definition of ac_fixed
} 

//////////////////////////////////////////////////////////////////////////////
//  ac_fixed 
//////////////////////////////////////////////////////////////////////////////

//enum ac_q_mode { AC_TRN, AC_RND, AC_TRN_ZERO, AC_RND_ZERO, AC_RND_INF, AC_RND_MIN_INF, AC_RND_CONV, AC_RND_CONV_ODD };
//enum ac_o_mode { AC_WRAP, AC_SAT, AC_SAT_ZERO, AC_SAT_SYM };

template<int W, int I, bool S=true, ac_q_mode Q=AC_TRN, ac_o_mode O=AC_WRAP>
class ac_fixed : private ac_private::iv<W, S> 
{
  enum {N=W, OP_W=AC_MAX(2, W)};

  template<int W2>
  struct rt_priv {
    enum {w_shiftl = AC_MAX(W+W2,1) };
    typedef ac_fixed<w_shiftl, I, S> shiftl;
  };

  typedef ac_private::iv<N, S> Base;

  inline Base &base() { return *this; }
  inline const Base &base() const { return *this; }

  inline void overflow_adjust(bool underflow, bool overflow) {
    if(O==AC_WRAP) {
      return;
    } 
    else if(O==AC_SAT_ZERO) {
      if((overflow || underflow))
        Base::value = 0;
    }
    else if(S) {
      if(overflow) {
        // 011..11 (W-1 1)
        Base::value = 1;
        Base::value <<= W-1;
        Base::value -= 1;
      } else if(underflow) {
        // 100..00 (W-1 0)
        Base::value = 1;
        Base::value <<= W-1;

        if(O == AC_SAT_SYM){
            Base::value |= 1;
        }
      }
    }
    else {
      if(overflow) {
        // 11..11 (W 1)
        Base::value = 0;
        Base::value = ~Base::value;
      } else if(underflow){
        // 0
        Base::value = 0;
      }
    }
  }
  // Left Value: Target Value (TV)
  // Right Value : Source Value (SV)
  // Extra Fixed Part: EFP
  //
  // Example 1:
  // ac_fixed<5,2, S> B = ac_fixed <12, 4, S> A
  // 
  //         B4 B3 . B2 B1 B0 |    |
  // A11 A10 A9 A8 . A7 A6 A5 | A4 | A3 A2 A1 A0
  //                          | qb |  r = (A3A2A1A0 == 0)
  //                          |  Extra Fixed Part
  //
  // 
  // Parameter:
  // s: SV is signed and negative
  // qb: MSB of EFP is 1
  // r: rest part of EFP without MSB is not 0 
  inline bool quantization_adjust(bool qb, bool r, bool s) {
    if(Q==AC_TRN){
      return false;
    }
    if(Q==AC_RND_ZERO){
      qb &= s || r; 
    }
    else if(Q==AC_RND_MIN_INF){ 
      qb &= r;
    }
    else if(Q==AC_RND_INF){
      qb &= !s || r;
    }
    else if(Q==AC_RND_CONV){ 
      qb &= (Base::value & 1) || r;
    }
    else if(Q==AC_RND_CONV_ODD){
      qb &= (!(Base::value & 1)) || r;
    }
    else if(Q==AC_TRN_ZERO){
      qb = s && ( qb || r );
    }
    return ac_private::ap_uadd_carry<N>(Base::value, qb, Base::value);
  }

  inline bool is_neg() const { return S && Base::value < 0; }

public:
  static const int width = W;
  static const int i_width = I;
  static const bool sign = S;
  static const ac_o_mode o_mode = O;
  static const ac_q_mode q_mode = Q;
  static const int e_width = 0;

  template<int W2, int I2, bool S2>
  struct rt {
    enum {
      F=W-I, 
      F2=W2-I2,
      mult_w = W+W2,
      mult_i = I+I2,
      mult_s = S||S2,
      plus_w = AC_MAX(I+(S2&&!S),I2+(S&&!S2))+1+AC_MAX(F,F2),
      plus_i = AC_MAX(I+(S2&&!S),I2+(S&&!S2))+1,
      plus_s = S||S2,
      minus_w = AC_MAX(I+(S2&&!S),I2+(S&&!S2))+1+AC_MAX(F,F2),
      minus_i = AC_MAX(I+(S2&&!S),I2+(S&&!S2))+1,
      minus_s = true,
      div_w = W+AC_MAX(W2-I2,0)+S2,
      div_i = I+(W2-I2)+S2,
      div_s = S||S2,
      logic_w = AC_MAX(I+(S2&&!S),I2+(S&&!S2))+AC_MAX(F,F2),
      logic_i = AC_MAX(I+(S2&&!S),I2+(S&&!S2)),
      logic_s = S||S2
    };
    typedef ac_fixed<mult_w, mult_i, mult_s> mult;
    typedef ac_fixed<plus_w, plus_i, plus_s> plus;
    typedef ac_fixed<minus_w, minus_i, minus_s> minus;
    typedef ac_fixed<logic_w, logic_i, logic_s> logic;
    typedef ac_fixed<div_w, div_i, div_s> div;
    typedef ac_fixed<W, I, S> arg1;
  };

  template<typename T>
  struct rt_T {
    typedef typename ac_private::map<T>::t map_T;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::mult mult;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::plus plus;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::minus minus;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::minus2 minus2;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::logic logic;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::div div;
    typedef typename ac_private::rt_ac_fixed_T<map_T>::template op1<W,I,S>::div2 div2;
    typedef ac_fixed<W, I, S> arg1;
  };

  struct rt_unary {
    enum {
      neg_w = W+1,
      neg_i = I+1,
      neg_s = true,
      mag_sqr_w = 2*W-S,
      mag_sqr_i = 2*I-S,
      mag_sqr_s = false,
      mag_w = W+S,
      mag_i = I+S,
      mag_s = false,
      leading_sign_w = ac::log2_ceil<W+!S>::val,
      leading_sign_s = false
    };
    typedef ac_int<leading_sign_w, leading_sign_s> leading_sign;
    typedef ac_fixed<neg_w, neg_i, neg_s> neg;
    typedef ac_fixed<mag_sqr_w, mag_sqr_i, mag_sqr_s> mag_sqr;
    typedef ac_fixed<mag_w, mag_i, mag_s> mag;
    template<unsigned N>
    struct set {
      enum { sum_w = W + ac::log2_ceil<N>::val, sum_i = (sum_w-W) + I, sum_s = S};
      typedef ac_fixed<sum_w, sum_i, sum_s> sum;
    };
  };

  ac_fixed(const ac_fixed &op): Base(op) { }

  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2> friend class ac_fixed;
  ac_fixed() {}

  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  inline ac_fixed (const ac_fixed<W2,I2,S2,Q2,O2> &op) {
    enum {N2=W2, F=W-I, F2=W2-I2, QUAN_INC = F2>F && !(Q==AC_TRN || (Q==AC_TRN_ZERO && !S2)) };
    bool carry = false;

    if(F2 == F){
      Base::operator =(op);
    }
    else if(F2 > F) {
      op.template const_shift_r<N,S,F2-F>(*this);

      if(Q!=AC_TRN && !(Q==AC_TRN_ZERO && !S2)) {
        bool qb = (F2-F > W2) ? ac_private::ap_less_zero(op.value)  :  ((bool) op[F2-F-1]);
        bool r = (F2 > F+1) ? !ac_private::ap_equal_zeros_to<AC_MIN(AC_MAX(1, F2-F-1), W2), W2>(op.value): false; 
        carry = quantization_adjust(qb, r, S2 && ac_private::ap_less_zero(op.value));
      }
    }
    else{
      op.template const_shift_l<N,S,F-F2>(*this);
    }

    if(O!=AC_WRAP && ((!S && S2) || I-S < I2-S2+(QUAN_INC || (S2 && O==AC_SAT_SYM && (O2 != AC_SAT_SYM || F2 > F) ))) ) { // saturation 
      /*
      not over flow condition:
      if(S && S2) true
      else if((!S) && S2) true
      else if( S && (!S2)) true
      else if( !S && !S2) carry ? !(Base::value == 0) : true
      */ 
     
      bool deleted_bits_zero = S || S2 || !(carry && Base::value == 0); 
      bool deleted_bits_one =  true; 
      bool neg_src;

      //Deal with Outer Overflow
      //          op[F2-F+W-1] 
      //                |
      // op[W2-1] - A A A A | X X X X X X      W2:10, I2:4, F2:6
      //                B B | Y Y Y            W :5,  I :2, F :3
      
      /*
      bool cond = false;
      condition to go to next if:
      if( S && S2 ) cond = (F2 - F + W  < W2);                // over flow: not all zeros from op[F2-F+W-1]
      else if( ((!S) && S2) cond = (F2 - F + W  < W2);        // only need to consider overflow, condition is not all 0s from op[F2-F+W] 
      else if( S && (!S2)) cond = (F2 - F + W - 1 < W2);      // only need to consider overflow, condition is not all 0s from op[F2-F+W-1]
      else if( (!S) && (!S2)) cond = (F2 - F + W  < W2);      // only need to consider overflow, condition is not all 0s from op[F2-F+W]
      */
      if( F2 - F + W  < W2 +  (S && (!S2)) ) { 
        bool all_ones = ac_private::ap_equal_ones_from<AC_MIN(AC_MAX(F2-F + W, 0),N2 - 1), N2>(op.value);
        // asign: YYY XXX.XXXXX W2 = 11  I2 = 6  F2 = 5   
        // to   :     ZZZ.AAAA  W  = 7   I  = 3  F  = 4
        // F2 - F + W  = 8
        // all_ones is true <==> YYY  are all ones 
       
        //carry == 1 if Base::value = 11..11 and now 00.000
        // not over flow
        const int START_POINT2 = AC_MAX(AC_MIN( (S ? (F2-F + W - 1) : (F2-F + W)) , N2 - 1), 0);
        deleted_bits_zero = deleted_bits_zero && (carry ? all_ones: ac_private::ap_equal_zeros_from<START_POINT2, N2>(op.value)); 
        
        // not under flow 
        deleted_bits_one = !S || !S2 || ( deleted_bits_one && (carry ? false: all_ones) );
        neg_src = S2 && ac_private::ap_less_zero(op.value) && !(carry & all_ones);
      }
      else{
        ac_private::ap_int<N> t = Base::value;
        neg_src = S2 && ac_private::ap_less_zero(op.value) && t < 0;
      }

      bool neg_trg = S && (bool) this->operator[](W-1);

      bool overflow = !neg_src && (neg_trg || !deleted_bits_zero);
      bool underflow = neg_src && (!neg_trg || !deleted_bits_one);

      if(O==AC_SAT_SYM && S && S2){
        underflow |= neg_src && (W > 1 ? ac_private::ap_equal_zeros_to<W-1,N>(Base::value) : true);
      }
      overflow_adjust(underflow, overflow);
    }
  }
  
  template<int W2, bool S2>
  inline ac_fixed (const ac_int<W2,S2> &op) {
    ac_fixed<W2,W2,S2> f_op;
    f_op.base().operator =(op);
    *this = f_op;
  }

  template<int W2>
  typename rt_priv<W2>::shiftl shiftl() const {
    typedef typename rt_priv<W2>::shiftl shiftl_t;
    shiftl_t r;
    Base::template const_shift_l<shiftl_t::N, S, W2>(r);
    return r;
  }

  template<int W2, bool S2>
  inline void set_op (const ac_int<W2,S2> &op, bool b) {
    Base::value = b ? -op.value : op.value;
  }
  
  inline void set_op_unsigned(const ac_private::ap_uint<OP_W> op){
    Base::value = op;
  }

  inline void set_op_signed(const ac_private::ap_int<OP_W> op){
    Base::value = op;
  }

  ac_private::ap_int<OP_W> get_op_signed(){
    ac_private::ap_int<OP_W> t = Base::value;
    return t;
  }

  ac_private::ap_uint<OP_W> get_op_unsigned(){
    ac_private::ap_uint<OP_W> t = Base::value;
    return t;
  }

  Ulong get_op_in_u64(){
    ac_int<W, S> op = 0;
    op.value = Base::value;
    return op.to_uint64();
  }

  inline ac_fixed( bool b ) { *this = (ac_int<1,false>) b; }
  inline ac_fixed( char b ) { *this = (ac_int<8,true>) b; }
  inline ac_fixed( signed char b ) { *this = (ac_int<8,true>) b; }
  inline ac_fixed( unsigned char b ) { *this = (ac_int<8,false>) b; }
  inline ac_fixed( signed short b ) { *this = (ac_int<16,true>) b; }
  inline ac_fixed( unsigned short b ) { *this = (ac_int<16,false>) b; }
  inline ac_fixed( signed int b ) { *this = (ac_int<32,true>) b; }
  inline ac_fixed( unsigned int b ) { *this = (ac_int<32,false>) b; }
  inline ac_fixed( signed long b ) { *this = (ac_int<ac_private::long_w,true>) b; }
  inline ac_fixed( unsigned long b ) { *this = (ac_int<ac_private::long_w,false>) b; }
  inline ac_fixed( Slong b ) { *this = (ac_int<64,true>) b; }
  inline ac_fixed( Ulong b ) { *this = (ac_int<64,false>) b; }

  inline ac_fixed( float d ) {
    unsigned s = *(int *)(&d);
    bool sign = (bool)((s >> 31) & 1);
    unsigned biased_exponent = unsigned((s >> __HLS_AC_FRAC_BITS_SINGLE) & 0xFF); // 8 bits
    unsigned base = (s & ((0X1L << __HLS_AC_FRAC_BITS_SINGLE) - 1)) | (0X1L << __HLS_AC_FRAC_BITS_SINGLE); //24 bits
    ac_int<__HLS_AC_W_SINGLE, true> t = base;
    ac_int<__HLS_AC_W_SINGLE, true> m = t << biased_exponent;
    __hls_ac_ret_single f_op;
    f_op.set_op(m, sign);
    __hls_ac_ret_single f_op2(f_op);
    *this = f_op2;
  }

  inline ac_fixed( double d ) {
    Ulong s = *(Ulong *)(&d);
    bool sign = (bool)((s >> 63) & 1);
    Ulong biased_exponent = Ulong((s >> __HLS_AC_FRAC_BITS_DOUBLE) & 0x7FFLL); // 11 bits
    Ulong base = (s & ((0X1LL << __HLS_AC_FRAC_BITS_DOUBLE) - 1LL)) | (0X1LL << __HLS_AC_FRAC_BITS_DOUBLE); //53 bits
    ac_int<__HLS_AC_W_DOUBLE, true> t = base;
    ac_int<__HLS_AC_W_DOUBLE, true> m = t << biased_exponent;
    __hls_ac_ret_double f_op;
    f_op.set_op(m, sign);
    __hls_ac_ret_double f_op2(f_op);
    *this = f_op2;
  }

  template<ac_special_val V>
  inline ac_fixed &set_val() {
    if(V == AC_VAL_DC) {
      ac_fixed r;
      Base::operator =(r);
    }
    else if(V == AC_VAL_0 || V == AC_VAL_MIN || V == AC_VAL_QUANTUM) {
      Base::operator =(0);
      if(S && V == AC_VAL_MIN) {
        //const unsigned rem = (W-1)&31;
        Base::value = 1;
        Base::value <<= W - 1;
        if(O == AC_SAT_SYM) {
          if(W == 1)
            Base::value = 0;
          else
            Base::value |= 1;
        }
      } else if(V == AC_VAL_QUANTUM) {
        Base::value = 1;
      }
    }
    else if(AC_VAL_MAX) {
      Base::operator =(-1);
      Base::value = 0;
      Base::value = ~Base::value;
      if(S){
        ac_private::ap_uint<N> t = Base::value;
        t >>= 1;
        Base::value = t;
      }
    }
    return *this;
  }

  // Explicit conversion functions to ac_int that captures all integer bits (bits are truncated)
  // Change from ref: smallest bits is 2!!!
  inline ac_int<AC_MAX(I,2),S> to_ac_int() const { return ((ac_fixed<AC_MAX(I,2),AC_MAX(I,2),S>) *this).template slc<AC_MAX(I,2)>(0); }

  // Explicit conversion functions to C built-in types -------------
  inline int to_int() const { return ((I-W) >= 32) ? 0 : (signed int) to_ac_int(); } 
  inline unsigned to_uint() const { return ((I-W) >= 32) ? 0 : (unsigned int) to_ac_int(); }
  inline long to_long() const { return ((I-W) >= ac_private::long_w) ? 0 : (signed long) to_ac_int(); } 
  inline unsigned long to_ulong() const { return ((I-W) >= ac_private::long_w) ? 0 : (unsigned long) to_ac_int(); } 
  inline Slong to_int64() const { return ((I-W) >= 64) ? 0 : (Slong) to_ac_int(); } 
  inline Ulong to_uint64() const { return ((I-W) >= 64) ? 0 : (Ulong) to_ac_int(); } 
  inline double to_double() const {
    static_assert(W <= 128, "ac_fixed to_double() does not support type larger than 128 bits");
    return ac_private::ldexpr<I-W>(Base::to_double());
  } 

  inline int length() const { return W; }
  #ifndef _HLS_EMBEDDED_PROFILE
  inline std::string to_string(ac_base_mode base_rep, bool sign_mag = false) const {
    ac_private::ap_uint<5> base = 10;
    switch(base_rep){
      case AC_DEC: base = 10; break;
      case AC_BIN: base = 2;  break;
      case AC_OCT: base = 8;  break;
      case AC_HEX: base = 16; break;
    }
    ac_private::ap_uint<5> base_diff = base / 2;

    // (I == W) will crash clang :(
    bool b = true;
    if( (I == W) && b ){
      ac_private::ap_int<W + 1> t = Base::value;
      return ac_private::to_string<W + 1>(t, base);
    }
    bool smaller = true;
    // Do not generate 0 value
    // I == W workaround, fix clang
    const int F = (!( (I > W) || (W > I))) ? 1 : (I > W ? I - W: W - I );
    if(I > W) smaller = false;
    ac_private::ap_uint<5 * F> boost = ac_private::ap_int_pow<5, F>(base_diff);
    if(!smaller){
      const ac_private::ap_uint<5> b = 2;
      boost = ac_private::ap_int_pow<5, F>(b);
    }
    std::string negative = "";
    ac_private::ap_int<W > tt = Base::value;
    ac_private::ap_int<W + 1> temp = Base::value;
    ac_private::ap_uint<W + 1> this_val = temp;
    if(temp < 0){
      this_val = -temp;
      negative = "-";
    }
    ac_private::ap_uint<5 * (F) + W + 1> boosted_v;
    boosted_v = ac_private::bit_multiply<5 * F,  W + 1>(boost, this_val);
    if(!smaller){
      return negative + ac_private::to_string_u<5 * F + W>(boosted_v, base);
    }
    std::string prefix_zeros(F, char('0'));
    std::string str1 = prefix_zeros + ac_private::to_string_u<5 * (F) + W + 1>(boosted_v, base);
    int size_fixed_part = F;
    int size_int_part = str1.size() - size_fixed_part;

    //should use stringstream here, but windows doesn't support it yet
    std::string buffer = "";
    bool not_zero = false;
    for(int i = 0; i < size_int_part; i++){
      if(str1[i] != '0'){
        not_zero = true;
      }
      if(not_zero){
        buffer = buffer + str1[i];

      }
    }
    std::string str_int_part = buffer;

    buffer = "";
    not_zero = false;
    for(int i = str1.size() - 1; i >= size_int_part; i--){
      if(str1[i] != '0'){
        not_zero = true;
      }
      if(not_zero){
        buffer = buffer + str1[i];
      }
    }
    std::string str_fixed_part= buffer;
    std::reverse(str_fixed_part.begin(), str_fixed_part.end());

    std::string result = negative + str_int_part;
    if(str_fixed_part != ""){
      result = result + "." + str_fixed_part;
    }
    if(result == ""){
      result = "0";
    }
    if(result != "0" && str_fixed_part == ""){
      result = result + ".0";
    }
    return result;
  }

  inline static std::string type_name() {
    const char *tf[] = {"false", "true" };
    const char *q[] = {"AC_TRN", "AC_RND", "AC_TRN_ZERO", "AC_RND_ZERO", "AC_RND_INF", "AC_RND_MIN_INF", "AC_RND_CONV", "AC_RND_CONV_ODD" };
    const char *o[] = {"AC_WRAP", "AC_SAT", "AC_SAT_ZERO", "AC_SAT_SYM" };
    std::string r = "ac_fixed<";
    r += ac_int<32,true>(W).to_string(AC_DEC) + ',';
    r += ac_int<32,true>(I).to_string(AC_DEC) + ',';
    r += tf[S];
    r += ',';
    r += q[Q];
    r += ',';
    r += o[O];
    r += '>';
    return r;
  }
  #endif //_HLS_EMBEDDED_PROFILE

  // Arithmetic : Binary ----------------------------------------------------
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::mult operator *( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    typename rt<W2,I2,S2>::mult r;
    Base::mult(op2, r);
    return r;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::plus operator +( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    typename rt<W2,I2,S2>::plus r;
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      Base::add(op2, r);
    else if(F > F2){
      Base::add(op2.template shiftl<shift_v>(), r);
    }
    else
      shiftl<shift_v>().add(op2, r); 
    return r;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::minus operator -( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    typename rt<W2,I2,S2>::minus r;
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      Base::sub(op2, r);
    else if(F > F2)
      Base::sub(op2.template shiftl<shift_v>(), r);
    else
      shiftl<shift_v>().sub(op2, r);
    return r;
  }
#if (defined(__GNUC__) && ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4 ) && !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::div operator /( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    typename rt<W2,I2,S2>::div r;
    enum { Num_w = W+AC_MAX(W2-I2,0), Num_i = I };
    ac_fixed<Num_w, Num_i, S> t = *this;
    t.template div<W2, S2, W+AC_MAX(W2-I2,0)+S2, S||S2>(op2, r);
    return r;
  }
#if (defined(__GNUC__) && ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 || __GNUC__ > 4 ) && !defined(__EDG__))
#pragma GCC diagnostic pop
#endif
  // Arithmetic assign  ------------------------------------------------------
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator *=( const ac_fixed<W2,I2,S2,Q2,O2> &op2) {
    *this = this->operator *(op2);
    return *this;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator +=( const ac_fixed<W2,I2,S2,Q2,O2> &op2) {
    *this = this->operator +(op2);
    return *this;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator -=( const ac_fixed<W2,I2,S2,Q2,O2> &op2) {
    *this = this->operator -(op2);
    return *this;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator /=( const ac_fixed<W2,I2,S2,Q2,O2> &op2) {
    *this = this->operator /(op2);
    return *this;
  }
  // increment/decrement by quantum (smallest difference that can be represented)
  // Arithmetic prefix increment, decrement ---------------------------------
  ac_fixed &operator ++() {
    ac_fixed<1,I-W+1,false> q;
    q.template set_val<AC_VAL_QUANTUM>();
    operator += (q);
    return *this;
  }
  ac_fixed &operator --() {
    ac_fixed<1,I-W+1,false> q;
    q.template set_val<AC_VAL_QUANTUM>();
    operator -= (q);
    return *this;
  }
  // Arithmetic postfix increment, decrement ---------------------------------
  const ac_fixed operator ++(int) {
    ac_fixed t = *this;
    ac_fixed<1,I-W+1,false> q;
    q.template set_val<AC_VAL_QUANTUM>();
    operator += (q); 
    return t;
  }
  const ac_fixed operator --(int) {
    ac_fixed t = *this;
    ac_fixed<1,I-W+1,false> q;
    q.template set_val<AC_VAL_QUANTUM>();
    operator -= (q);
    return t;
  }
  // Arithmetic Unary --------------------------------------------------------
  ac_fixed operator +() {
    return *this;
  }
  typename rt_unary::neg operator -() const {
    typename rt_unary::neg r;
    Base::neg(r);
    return r;
  }
  // ! ------------------------------------------------------------------------
  bool operator ! () const {
    return Base::equal_zero(); 
  }

  // Bitwise (arithmetic) unary: complement  -----------------------------
  ac_fixed<W+!S, I+!S, true> operator ~() const {
    ac_fixed<W+!S, I+!S, true> r;
    Base::bitwise_complement(r);
    return r;
  }
  // Bitwise (not arithmetic) bit complement  -----------------------------
  ac_fixed<W, I, false> bit_complement() const {
    ac_fixed<W, I, false> r;
    Base::bitwise_complement(r);
    return r;
  }
  // Bitwise (not arithmetic): and, or, xor ----------------------------------
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::logic operator &( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    typename rt<W2,I2,S2>::logic r;
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      Base::bitwise_and(op2, r);
    else if(F > F2)
      Base::bitwise_and(op2.template shiftl<shift_v>(), r);
    else
      shiftl<shift_v>().bitwise_and(op2, r);
    return r;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::logic operator |( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    typename rt<W2,I2,S2>::logic r;
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      Base::bitwise_or(op2, r);
    else if(F > F2)
      Base::bitwise_or(op2.template shiftl<shift_v>(), r);
    else
      shiftl<shift_v>().bitwise_or(op2, r);
    return r;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  typename rt<W2,I2,S2>::logic operator ^( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    typename rt<W2,I2,S2>::logic r;
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      Base::bitwise_xor(op2, r);
    else if(F > F2)
      Base::bitwise_xor(op2.template shiftl<shift_v>(), r);
    else
      shiftl<shift_v>().bitwise_xor(op2, r);
    return r;
  }
  // Bitwise assign (not arithmetic): and, or, xor ----------------------------
  
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator &= ( const ac_fixed<W2,I2,S2,Q2,O2> &op2 ) {
    *this = this->operator &(op2);
    return *this;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator |= ( const ac_fixed<W2,I2,S2,Q2,O2> &op2 ) {
    *this = this->operator |(op2);
    return *this;
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  ac_fixed &operator ^= ( const ac_fixed<W2,I2,S2,Q2,O2> &op2 ) {
    *this = this->operator ^(op2);
    return *this;
  }
  
  // Shift (result constrained by left operand) -------------------------------
  template<int W2>
  ac_fixed operator << ( const ac_int<W2,true> &op2 ) const {
    // currently not written to overflow or quantize (neg shift)
    ac_fixed r;
    Base::shift_l2(op2.to_int(), r);
    return r;
  }
  template<int W2>
  ac_fixed operator << ( const ac_int<W2,false> &op2 ) const {
    // currently not written to overflow
    ac_fixed r;
    Base::shift_l(op2.to_uint(), r);
    return r;
  }
  template<int W2>
  ac_fixed operator >> ( const ac_int<W2,true> &op2 ) const {
    // currently not written to quantize or overflow (neg shift)
    ac_fixed r;
    Base::shift_r2(op2.to_int(), r);
    return r;
  }
  template<int W2>
  ac_fixed operator >> ( const ac_int<W2,false> &op2 ) const {
    // currently not written to quantize 
    ac_fixed r;
    Base::shift_r(op2.to_uint(), r);
    return r;
  }
  // Shift assign ------------------------------------------------------------
  template<int W2>
  ac_fixed operator <<= ( const ac_int<W2,true> &op2 ) {
    // currently not written to overflow or quantize (neg shift)
    Base r;
    Base::shift_l2(op2.to_int(), r);
    Base::operator=(r);
    return *this;
  }
  template<int W2>
  ac_fixed operator <<= ( const ac_int<W2,false> &op2 ) {
    // currently not written to overflow
    Base r;
    Base::shift_l(op2.to_uint(), r);
    Base::operator=(r);
    return *this;
  }
  template<int W2>
  ac_fixed operator >>= ( const ac_int<W2,true> &op2 ) {
    // currently not written to quantize or overflow (neg shift)
    Base r;
    Base::shift_r2(op2.to_int(), r);
    Base::operator=(r);
    return *this;
  }
  template<int W2>
  ac_fixed operator >>= ( const ac_int<W2,false> &op2 ) {
    // currently not written to quantize 
    Base r;
    Base::shift_r(op2.to_uint(), r);
    Base::operator=(r);
    return *this;
  }
  // Relational ---------------------------------------------------------------
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator == ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return Base::equal(op2);
    else if(F > F2)
      return Base::equal(op2.template shiftl<shift_v>());
    else
      return shiftl<shift_v>().equal(op2);
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator != ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return ! Base::equal(op2);
    else if(F > F2)
      return ! Base::equal(op2.template shiftl<shift_v>());
    else
      return ! shiftl<shift_v>().equal(op2);
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator < ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return Base::less_than(op2);
    else if(F > F2)
      return Base::less_than(op2.template shiftl<shift_v>());
    else
      return shiftl<shift_v>().less_than(op2);
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator >= ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return ! Base::less_than(op2);
    else if(F > F2)
      return ! Base::less_than(op2.template shiftl<shift_v>());
    else
      return ! shiftl<shift_v>().less_than(op2);
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator > ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return Base::greater_than(op2);
    else if(F > F2)
      return Base::greater_than(op2.template shiftl<shift_v>());
    else
      return shiftl<shift_v>().greater_than(op2); 
  }
  template<int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
  bool operator <= ( const ac_fixed<W2,I2,S2,Q2,O2> &op2) const {
    enum { F=W-I, F2=W2-I2 };
    const int shift_v = AC_MAX(F - F2, F2 - F);
    if(F == F2)
      return ! Base::greater_than(op2);
    else if(F > F2)
      return ! Base::greater_than(op2.template shiftl<shift_v>());
    else
      return ! shiftl<shift_v>().greater_than(op2);
  }

  /*
  bool operator == ( double d) const {
    if(is_neg() != (d < 0.0))
      return false;
    double di = ac_private::ldexpr<-(I+!S+((32-W-!S)&31))>(d);
    bool overflow, qb, r;
    ac_fixed<W,I,S> t;
    t.conv_from_fraction(di, &qb, &r, &overflow);
    if(qb || r || overflow)
      return false;
    return operator == (t);
  }
  bool operator != ( double d) const {
    return !operator == ( d );
  }
  bool operator < ( double d) const {
    if(is_neg() != (d < 0.0))
      return is_neg();
    double di = ac_private::ldexpr<-(I+!S+((32-W-!S)&31))>(d);
    bool overflow, qb, r;
    ac_fixed<W,I,S> t;
    t.conv_from_fraction(di, &qb, &r, &overflow);
    if(is_neg() && overflow)
      return false;
    return (!is_neg() && overflow) || ((qb || r) && operator <= (t)) || operator < (t);
  }
  bool operator >= ( double d) const {
    return !operator < ( d );
  }
  bool operator > ( double d) const {
    if(is_neg() != (d < 0.0))
      return !is_neg();
    double di = ac_private::ldexpr<-(I+!S+((32-W-!S)&31))>(d);
    bool overflow, qb, r;
    ac_fixed<W,I,S> t;
    t.conv_from_fraction(di, &qb, &r, &overflow);
    if(!is_neg() && overflow )
      return false;
    return (is_neg() && overflow) || operator > (t);
  }
  bool operator <= ( double d) const {
    return !operator > ( d );
  }
  */

  // Bit and Slice Select -----------------------------------------------------
  template<int WS, int WX, bool SX>
  inline ac_int<WS,S> slc(const ac_int<WX,SX> &index) const {
    ac_int<W, S> op = 0;
    AC_ASSERT(index >= 0, "Attempting to read slc with negative indices");
    ac_int<WX-SX, false> uindex = index;
    Base::shift_r(uindex.to_uint(), op);
    ac_int<WS,S> r = op;
    return r; 
  }

  template<int WS>
  inline ac_int<WS,S> slc(signed index) const {
    ac_int<W, S> op = 0;
    AC_ASSERT(index >= 0, "Attempting to read slc with negative indices");
    unsigned uindex = index & ((unsigned)~0 >> 1);
    Base::shift_r(uindex, op);
    ac_int<WS,S> r = op;
    return r; 
  }

  template<int WS>
  inline ac_int<WS,S> slc(unsigned uindex) const {
    ac_int<W, S> op = 0;
    Base::shift_r(uindex, op);
    ac_int<WS,S> r = op;
    return r; 
  }

  template<int W2, bool S2, int WX, bool SX>
  inline ac_fixed &set_slc(const ac_int<WX,SX> lsb, const ac_int<W2,S2> &slc) {
    AC_ASSERT(lsb.to_int() + W2 <= W && lsb.to_int() >= 0, "Out of bounds set_slc");
    ac_int<WX-SX, false> ulsb = lsb;
    Base::set_slc(ulsb.to_uint(), W2, (ac_int<W2,false>) slc);
    return *this;
  }

  template<int W2, bool S2>
  inline ac_fixed &set_slc(signed lsb, const ac_int<W2,S2> &slc) {
    AC_ASSERT(lsb + W2 <= W && lsb >= 0, "Out of bounds set_slc");
    unsigned ulsb = lsb & ((unsigned)~0 >> 1);
    Base::set_slc(ulsb, W2, (ac_int<W2,false>) slc);

    return *this;
  }

  template<int W2, bool S2>
  inline ac_fixed &set_slc(unsigned ulsb, const ac_int<W2,S2> &slc) {
    AC_ASSERT(ulsb + W2 <= W, "Out of bounds set_slc");
    Base::set_slc(ulsb, W2, (ac_int<W2,false>) slc);
    return *this;
  }

  class ac_bitref {
    ac_fixed &d_bv;
    unsigned d_index;
  public:
    ac_bitref( ac_fixed *bv, unsigned index=0 ) : d_bv(*bv), d_index(index) {}\

    operator bool () const { return (d_index < W) ? (bool)(d_bv.value >> (d_index) & 1) : 0; }

    inline ac_bitref operator = ( int val ) {
      // lsb of int (val&1) is written to bit
      /*
      if(d_index < W) {
        int *pval = &d_bv.v[d_index>>5];
        *pval ^= (*pval ^ (val << (d_index&31) )) & 1 << (d_index&31);
        d_bv.bit_adjust();   // in case sign bit was assigned
      }
      return *this;
      */
      if(d_index < W) {
        ac_private::ap_int<W+1> temp1 = d_bv.value;
        ac_private::ap_int<W+1> temp2 = val;
        temp2 <<= d_index;
        temp1 ^= temp2;
        temp2 = 1;
        temp2 <<= d_index;
        temp1 &= temp2;
        d_bv.value ^= temp1;
      }
      return *this;
    }

    template<int W2, bool S2>
    inline ac_bitref operator = ( const ac_int<W2,S2> &val ) {
      return operator =(val.to_int());
    }
    inline ac_bitref operator = ( const ac_bitref &val ) {
      return operator =((int) (bool) val);
    }
  };
                                                                                                             
  ac_bitref operator [] ( unsigned int uindex) {
    AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
    ac_bitref bvh( this, uindex );
    return bvh;
  }
  ac_bitref operator [] ( int index) {
    AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
    AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
    unsigned uindex = index & ((unsigned)~0 >> 1);
    ac_bitref bvh( this, uindex );
    return bvh;
  }
  template<int W2, bool S2>
  ac_bitref operator [] ( const ac_int<W2,S2> &index) {
    AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
    AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
    ac_int<W2-S2,false> uindex = index;
    ac_bitref bvh( this, uindex.to_uint() );
    return bvh;
  }

  bool operator [] ( unsigned int uindex) const {
    AC_ASSERT(uindex < W, "Attempting to read bit beyond MSB");
    return (uindex < W) ? (bool)(Base::value >> uindex & 1) : 0;
  }
  bool operator [] ( int index) const {
    AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
    AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
    unsigned uindex = index & ((unsigned)~0 >> 1);
    return (uindex < W) ? (bool)(Base::value >> uindex & 1) : 0;
  }
  template<int W2, bool S2>
  bool operator [] ( const ac_int<W2,S2> &index) const {
    AC_ASSERT(index >= 0, "Attempting to read bit with negative index");
    AC_ASSERT(index < W, "Attempting to read bit beyond MSB");
    ac_int<W2-S2,false> uindex = index;
    return (uindex < W) ? (bool)(Base::value >> uindex.to_uint() & 1) : 0;
  }
  /*
  typename rt_unary::leading_sign leading_sign() const {
    unsigned ls = Base::leading_bits(S & (Base::value < 0)) - (32*N - W)-S;
    return ls;
  }
  typename rt_unary::leading_sign leading_sign(bool &all_sign) const {
    unsigned ls = Base::leading_bits(S & (Base::value < 0)) - (32*N - W)-S;
    all_sign = (ls == W-S);
    return ls;
  }
  // returns false if number is denormal
  template<int WE, bool SE>
  bool normalize(ac_int<WE,SE> &exp) {
    ac_int<W,S> m = this->template slc<W>(0);
    bool r = m.normalize(exp);
    this->set_slc(0,m);
    return r;
  }
  // returns false if number is denormal, minimum exponent is reserved (usually for encoding special values/errors)
  template<int WE, bool SE>
  bool normalize_RME(ac_int<WE,SE> &exp) {
    ac_int<W,S> m = this->template slc<W>(0);
    bool r = m.normalize_RME(exp);
    this->set_slc(0,m);
    return r;
  }
  */
  inline void bit_fill_hex(const char *str) {
    // Zero Pads if str is too short, throws ms bits away if str is too long
    // Asserts if anything other than 0-9a-fA-F is encountered
    ac_int<W,S> x = 0;
    x.bit_fill_hex(str);
    set_slc(0, x);
  }
  template<int N>
  inline void bit_fill(const int (&ivec)[N], bool bigendian=true) {
    // bit_fill from integer vector
    //   if W > N*32, missing most significant bits are zeroed
    //   if W < N*32, additional bits in ivec are ignored (no overflow checking)
    //
    // Example:  
    //   ac_fixed<80,40,false> x;    int vec[] = { 0xffffa987, 0x6543210f, 0xedcba987 };
    //   x.bit_fill(vec);   // vec[0] fill bits 79-64 
    ac_int<W,S> x = 0;
    x.bit_fill(ivec, bigendian);
    set_slc(0, x);
  }
};

namespace ac {
  template<typename T>
  struct ac_fixed_represent {
    enum { t_w = ac_private::c_type_params<T>::W, t_i = t_w, t_s = ac_private::c_type_params<T>::S };
    typedef ac_fixed<t_w,t_i,t_s> type;
  };
  template<> struct ac_fixed_represent<float> {};
  template<> struct ac_fixed_represent<double> {};
  template<int W, bool S>
  struct ac_fixed_represent< ac_int<W,S> > {
    typedef ac_fixed<W,W,S> type;
  };
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
  struct ac_fixed_represent< ac_fixed<W,I,S,Q,O> > {
    typedef ac_fixed<W,I,S,Q,O> type;
  };
}

namespace ac_private {
  // with T == ac_fixed
  template<int W2, int I2, bool S2>
  struct rt_ac_fixed_T< ac_fixed<W2,I2,S2> > {
    typedef ac_fixed<W2,I2,S2> fx2_t;
    template<int W, int I, bool S>
    struct op1 {
      typedef ac_fixed<W,I,S> fx_t;
      typedef typename fx_t::template rt<W2,I2,S2>::mult mult;
      typedef typename fx_t::template rt<W2,I2,S2>::plus plus;
      typedef typename fx_t::template rt<W2,I2,S2>::minus minus;
      typedef typename fx2_t::template rt<W,I,S>::minus minus2;
      typedef typename fx_t::template rt<W2,I2,S2>::logic logic;
      typedef typename fx_t::template rt<W2,I2,S2>::div div;
      typedef typename fx2_t::template rt<W,I,S>::div div2;
    };
  };
  // with T == ac_int
  template<int W2, bool S2>
  struct rt_ac_fixed_T< ac_int<W2,S2> > {
    typedef ac_fixed<W2,W2,S2> fx2_t;
    template<int W, int I, bool S>
    struct op1 {
      typedef ac_fixed<W,I,S> fx_t;
      typedef typename fx_t::template rt<W2,W2,S2>::mult mult;
      typedef typename fx_t::template rt<W2,W2,S2>::plus plus;
      typedef typename fx_t::template rt<W2,W2,S2>::minus minus;
      typedef typename fx2_t::template rt<W,I,S>::minus minus2;
      typedef typename fx_t::template rt<W2,W2,S2>::logic logic;
      typedef typename fx_t::template rt<W2,W2,S2>::div div;
      typedef typename fx2_t::template rt<W,I,S>::div div2;
    };
  };

  template<typename T>
  struct rt_ac_fixed_T< c_type<T> > {
    typedef typename ac::ac_fixed_represent<T>::type fx2_t;
    enum { W2 = fx2_t::width, I2 = W2, S2 = fx2_t::sign };
    template<int W, int I, bool S>
    struct op1 {
      typedef ac_fixed<W,I,S> fx_t;
      typedef typename fx_t::template rt<W2,W2,S2>::mult mult;
      typedef typename fx_t::template rt<W2,W2,S2>::plus plus;
      typedef typename fx_t::template rt<W2,W2,S2>::minus minus;
      typedef typename fx2_t::template rt<W,I,S>::minus minus2;
      typedef typename fx_t::template rt<W2,W2,S2>::logic logic;
      typedef typename fx_t::template rt<W2,W2,S2>::div div;
      typedef typename fx2_t::template rt<W,I,S>::div div2;
    };
  };
}


// Specializations for constructors on integers that bypass bit adjusting
//  and are therefore more efficient
//this use ap_int<1>
//template<> inline ac_fixed<1,1,true,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b ? -1 : 0; }

template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( signed char b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned char b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( signed short b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned short b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( signed int b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned int b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( signed long b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned long b ) { value = b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( Ulong b ) { value = (int) b&1; }
template<> inline ac_fixed<1,1,false,AC_TRN,AC_WRAP>::ac_fixed( Slong b ) { value = (int) b&1; }

template<> inline ac_fixed<8,8,true,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b; }
template<> inline ac_fixed<8,8,false,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b; }
template<> inline ac_fixed<8,8,true,AC_TRN,AC_WRAP>::ac_fixed( signed char b ) { value = b; }
template<> inline ac_fixed<8,8,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned char b ) { value = b; }
template<> inline ac_fixed<8,8,true,AC_TRN,AC_WRAP>::ac_fixed( unsigned char b ) { value = (signed char) b; }
template<> inline ac_fixed<8,8,false,AC_TRN,AC_WRAP>::ac_fixed( signed char b ) { value = (unsigned char) b; }

template<> inline ac_fixed<16,16,true,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b; }
template<> inline ac_fixed<16,16,false,AC_TRN,AC_WRAP>::ac_fixed( bool b ) { value = b; }
template<> inline ac_fixed<16,16,true,AC_TRN,AC_WRAP>::ac_fixed( signed char b ) { value = b; }
template<> inline ac_fixed<16,16,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned char b ) { value = b; }
template<> inline ac_fixed<16,16,true,AC_TRN,AC_WRAP>::ac_fixed( unsigned char b ) { value = b; }
template<> inline ac_fixed<16,16,false,AC_TRN,AC_WRAP>::ac_fixed( signed char b ) { value = (unsigned short) b; }
template<> inline ac_fixed<16,16,true,AC_TRN,AC_WRAP>::ac_fixed( signed short b ) { value = b; }
template<> inline ac_fixed<16,16,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned short b ) { value = b; }
template<> inline ac_fixed<16,16,true,AC_TRN,AC_WRAP>::ac_fixed( unsigned short b ) { value = (signed short) b; }
template<> inline ac_fixed<16,16,false,AC_TRN,AC_WRAP>::ac_fixed( signed short b ) { value = (unsigned short) b; }

template<> inline ac_fixed<32,32,true,AC_TRN,AC_WRAP>::ac_fixed( signed int b ) { value = b; }
template<> inline ac_fixed<32,32,true,AC_TRN,AC_WRAP>::ac_fixed( unsigned int b ) { value = b; }
template<> inline ac_fixed<32,32,false,AC_TRN,AC_WRAP>::ac_fixed( signed int b ) { value = b; }
template<> inline ac_fixed<32,32,false,AC_TRN,AC_WRAP>::ac_fixed( unsigned int b ) { value = b; }

template<> inline ac_fixed<32,32,true,AC_TRN,AC_WRAP>::ac_fixed( Slong b ) { value = (int) b; }
template<> inline ac_fixed<32,32,true,AC_TRN,AC_WRAP>::ac_fixed( Ulong b ) { value = (int) b; }
template<> inline ac_fixed<32,32,false,AC_TRN,AC_WRAP>::ac_fixed( Slong b ) { value = (int) b; }
template<> inline ac_fixed<32,32,false,AC_TRN,AC_WRAP>::ac_fixed( Ulong b ) { value = (int) b; }

template<> inline ac_fixed<64,64,true,AC_TRN,AC_WRAP>::ac_fixed( Slong b ) { value = (int) b; }
template<> inline ac_fixed<64,64,true,AC_TRN,AC_WRAP>::ac_fixed( Ulong b ) { value = (int) b; }
template<> inline ac_fixed<64,64,false,AC_TRN,AC_WRAP>::ac_fixed( Slong b ) { value = (int) b; }
template<> inline ac_fixed<64,64,false,AC_TRN,AC_WRAP>::ac_fixed( Ulong b ) { value = (int) b; }


// Stream --------------------------------------------------------------------
#ifndef _HLS_EMBEDDED_PROFILE

template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
inline std::ostream& operator << (std::ostream &os, const ac_fixed<W,I,S,Q,O> &x) {
  os << x.to_string(AC_DEC);
  return os;
}

#endif //_HLS_EMBEDDED_PROFILE

// Macros for Binary Operators with C Integers --------------------------------------------

#define FX_BIN_OP_WITH_INT_2I(BIN_OP, C_TYPE, WI, SI, RTYPE)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline typename ac_fixed<W,I,S>::template rt<WI,WI,SI>::RTYPE operator BIN_OP ( const ac_fixed<W,I,S,Q,O> &op, C_TYPE i_op) {  \
    return op.operator BIN_OP (ac_int<WI,SI>(i_op));  \
  }

#define FX_BIN_OP_WITH_INT(BIN_OP, C_TYPE, WI, SI, RTYPE)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline typename ac_fixed<WI,WI,SI>::template rt<W,I,S>::RTYPE operator BIN_OP ( C_TYPE i_op, const ac_fixed<W,I,S,Q,O> &op) {  \
    return ac_fixed<WI,WI,SI>(i_op).operator BIN_OP (op);  \
  } \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline typename ac_fixed<W,I,S>::template rt<WI,WI,SI>::RTYPE operator BIN_OP ( const ac_fixed<W,I,S,Q,O> &op, C_TYPE i_op) {  \
    return op.operator BIN_OP (ac_fixed<WI,WI,SI>(i_op));  \
  }

#define FX_REL_OP_WITH_INT(REL_OP, C_TYPE, W2, S2)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline bool operator REL_OP ( const ac_fixed<W,I,S,Q,O> &op, C_TYPE op2) {  \
    return op.operator REL_OP (ac_fixed<W2,W2,S2>(op2));  \
  }  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline bool operator REL_OP ( C_TYPE op2, const ac_fixed<W,I,S,Q,O> &op) {  \
    return ac_fixed<W2,W2,S2>(op2).operator REL_OP (op);  \
  }

#define FX_ASSIGN_OP_WITH_INT_2(ASSIGN_OP, C_TYPE, W2, S2)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline ac_fixed<W,I,S,Q,O> &operator ASSIGN_OP ( ac_fixed<W,I,S,Q,O> &op, C_TYPE op2) {  \
    return op.operator ASSIGN_OP (ac_fixed<W2,W2,S2>(op2));  \
  }

#define FX_ASSIGN_OP_WITH_INT_2I(ASSIGN_OP, C_TYPE, W2, S2)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O> \
  inline ac_fixed<W,I,S> operator ASSIGN_OP ( ac_fixed<W,I,S,Q,O> &op, C_TYPE op2) {  \
    return op.operator ASSIGN_OP (ac_int<W2,S2>(op2));  \
  }

#define FX_OPS_WITH_INT(C_TYPE, WI, SI) \
  FX_BIN_OP_WITH_INT(*, C_TYPE, WI, SI, mult) \
  FX_BIN_OP_WITH_INT(+, C_TYPE, WI, SI, plus) \
  FX_BIN_OP_WITH_INT(-, C_TYPE, WI, SI, minus) \
  FX_BIN_OP_WITH_INT(/, C_TYPE, WI, SI, div) \
  FX_BIN_OP_WITH_INT_2I(>>, C_TYPE, WI, SI, arg1) \
  FX_BIN_OP_WITH_INT_2I(<<, C_TYPE, WI, SI, arg1) \
  FX_BIN_OP_WITH_INT(&, C_TYPE, WI, SI, logic) \
  FX_BIN_OP_WITH_INT(|, C_TYPE, WI, SI, logic) \
  FX_BIN_OP_WITH_INT(^, C_TYPE, WI, SI, logic) \
  \
  FX_REL_OP_WITH_INT(==, C_TYPE, WI, SI) \
  FX_REL_OP_WITH_INT(!=, C_TYPE, WI, SI) \
  FX_REL_OP_WITH_INT(>, C_TYPE, WI, SI) \
  FX_REL_OP_WITH_INT(>=, C_TYPE, WI, SI) \
  FX_REL_OP_WITH_INT(<, C_TYPE, WI, SI) \
  FX_REL_OP_WITH_INT(<=, C_TYPE, WI, SI) \
  \
  FX_ASSIGN_OP_WITH_INT_2(+=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(-=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(*=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(/=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2I(>>=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2I(<<=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(&=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(|=, C_TYPE, WI, SI) \
  FX_ASSIGN_OP_WITH_INT_2(^=, C_TYPE, WI, SI)


// --------------------------------------- End of Macros for Binary Operators with C Integers 

namespace ac {
  
  namespace ops_with_other_types {
    // Binary Operators with C Integers --------------------------------------------
    FX_OPS_WITH_INT(bool, 1, false)
    FX_OPS_WITH_INT(char, 8, true)
    FX_OPS_WITH_INT(signed char, 8, true)
    FX_OPS_WITH_INT(unsigned char, 8, false)
    FX_OPS_WITH_INT(short, 16, true)
    FX_OPS_WITH_INT(unsigned short, 16, false)
    FX_OPS_WITH_INT(int, 32, true)
    FX_OPS_WITH_INT(unsigned int, 32, false)
    FX_OPS_WITH_INT(long, ac_private::long_w, true)
    FX_OPS_WITH_INT(unsigned long, ac_private::long_w, false)
    FX_OPS_WITH_INT(Slong, 64, true)
    FX_OPS_WITH_INT(Ulong, 64, false)
    // -------------------------------------- End of Binary Operators with Integers 
  }  // ops_with_other_types namespace

} // ac namespace


// Macros for Binary Operators with ac_int --------------------------------------------


#define FX_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline typename ac_fixed<WI,WI,SI>::template rt<W,I,S>::RTYPE operator BIN_OP ( const ac_int<WI,SI> &i_op, const ac_fixed<W,I,S,Q,O> &op) {  \
    return ac_fixed<WI,WI,SI>(i_op).operator BIN_OP (op);  \
  }

#define FX_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline typename ac_fixed<W,I,S>::template rt<WI,WI,SI>::RTYPE operator BIN_OP ( const ac_fixed<W,I,S,Q,O> &op, const ac_int<WI,SI> &i_op) {  \
    return op.operator BIN_OP (ac_fixed<WI,WI,SI>(i_op));  \
  }

#define FX_BIN_OP_WITH_AC_INT(BIN_OP, RTYPE)  \
  FX_BIN_OP_WITH_AC_INT_1(BIN_OP, RTYPE) \
  FX_BIN_OP_WITH_AC_INT_2(BIN_OP, RTYPE)

#define FX_REL_OP_WITH_AC_INT(REL_OP)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline bool operator REL_OP ( const ac_fixed<W,I,S,Q,O> &op, const ac_int<WI,SI> &op2) {  \
    return op.operator REL_OP (ac_fixed<WI,WI,SI>(op2));  \
  }  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline bool operator REL_OP ( ac_int<WI,SI> &op2, const ac_fixed<W,I,S,Q,O> &op) {  \
    return ac_fixed<WI,WI,SI>(op2).operator REL_OP (op);  \
  }

#define FX_ASSIGN_OP_WITH_AC_INT(ASSIGN_OP)  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline ac_fixed<W,I,S,Q,O> &operator ASSIGN_OP ( ac_fixed<W,I,S,Q,O> &op, const ac_int<WI,SI> &op2) {  \
    return op.operator ASSIGN_OP (ac_fixed<WI,WI,SI>(op2));  \
  }  \
  template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int WI, bool SI> \
  inline ac_int<WI,SI> &operator ASSIGN_OP ( ac_int<WI,SI> &op, const ac_fixed<W,I,S,Q,O> &op2) {  \
    return op.operator ASSIGN_OP (op2.to_ac_int());  \
  }  

// -------------------------------------------- End of Macros for Binary Operators with ac_int

namespace ac {
  namespace ops_with_other_types {
    // Binary Operators with ac_int --------------------------------------------
    
    FX_BIN_OP_WITH_AC_INT(*, mult)
    FX_BIN_OP_WITH_AC_INT(+, plus)
    FX_BIN_OP_WITH_AC_INT(-, minus)
    FX_BIN_OP_WITH_AC_INT(/, div)
    FX_BIN_OP_WITH_AC_INT(&, logic)
    FX_BIN_OP_WITH_AC_INT(|, logic)
    FX_BIN_OP_WITH_AC_INT(^, logic)

    FX_REL_OP_WITH_AC_INT(==)
    FX_REL_OP_WITH_AC_INT(!=)
    FX_REL_OP_WITH_AC_INT(>)
    FX_REL_OP_WITH_AC_INT(>=)
    FX_REL_OP_WITH_AC_INT(<)
    FX_REL_OP_WITH_AC_INT(<=)

    FX_ASSIGN_OP_WITH_AC_INT(+=)
    FX_ASSIGN_OP_WITH_AC_INT(-=)
    FX_ASSIGN_OP_WITH_AC_INT(*=)
    FX_ASSIGN_OP_WITH_AC_INT(/=)
    FX_ASSIGN_OP_WITH_AC_INT(%=)
    FX_ASSIGN_OP_WITH_AC_INT(&=)
    FX_ASSIGN_OP_WITH_AC_INT(|=)
    FX_ASSIGN_OP_WITH_AC_INT(^=)
  
    // -------------------------------------- End of Binary Operators with ac_int 

    // Relational Operators with double --------------------------------------
    
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator == ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator == (op); 
    }
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator != ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator != (op); 
    }
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator > ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator < (op); 
    }
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator < ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator > (op); 
    }
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator <= ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator >= (op); 
    }
    template<int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
    inline bool operator >= ( double op, const ac_fixed<W,I,S,Q,O> &op2) {
      return op2.operator <= (op); 
    }
    // -------------------------------------- End of Relational Operators with double 

  }  // ops_with_other_types namespace
} // ac namespace

using namespace ac::ops_with_other_types;

// Global templatized functions for easy initialization to special values
template<ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
inline ac_fixed<W,I,S,Q,O> value(ac_fixed<W,I,S,Q,O>) {
  ac_fixed<W,I,S> r;
  return r.template set_val<V>();
}

namespace ac {
// PUBLIC FUNCTIONS
// function to initialize (or uninitialize) arrays

  /*
  template<ac_special_val V, int W, int I, bool S, ac_q_mode Q, ac_o_mode O> 
  inline bool init_array(ac_fixed<W,I,S,Q,O> *a, int n) {
    ac_fixed<W,I,S> t = value<V>(*a);
    for(int i=0; i < n; i++)
      a[i] = t;
    return true;
  }

  inline ac_fixed<54,2,true> frexp_d(double d, ac_int<11,true> &exp) {
    enum {Min_Exp = -1022, Max_Exp = 1023, Mant_W = 52, Denorm_Min_Exp = Min_Exp - Mant_W};
    if(!d) {
      exp = 0;
      return 0;
    }
    int exp_i;
    double f0 = frexp(d, &exp_i); 
    AC_ASSERT(exp_i <= Max_Exp+1, "Exponent greater than standard double-precision float exponent max (+1024). It is probably an extended double");
    AC_ASSERT(exp_i >= Denorm_Min_Exp+1, "Exponent less than standard double-precision float exponent min (-1021). It is probably an extended double");
    exp_i--;
    int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : (exp_i > Min_Exp && f0 < 0 && f0 >= -0.5) ? -1 : 0; 
    exp = exp_i + rshift;
    ac_int<Mant_W+2,true> f_i = f0 * ((Ulong) 1 << (Mant_W + 1 -rshift));
    ac_fixed<Mant_W+2,2,true> r;
    r.set_slc(0, f_i);
    return r; 
  }
  inline ac_fixed<25,2,true> frexp_f(float f, ac_int<8,true> &exp) {
    enum {Min_Exp = -126, Max_Exp = 127, Mant_W = 23, Denorm_Min_Exp = Min_Exp - Mant_W};
    if(!f) {
      exp = 0;
      return 0;
    }
    int exp_i;
    float f0 = frexpf(f, &exp_i); 
    AC_ASSERT(exp_i <= Max_Exp+1, "Exponent greater than standard single-precision float exponent max (+128). It is probably an extended float");
    AC_ASSERT(exp_i >= Denorm_Min_Exp+1, "Exponent less than standard single-precision float exponent min (-125). It is probably an extended float");
    exp_i--;
    int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : (exp_i >= Min_Exp && f0 < 0 && f0 >= -0.5) ? -1 : 0; 
    exp = exp_i + rshift;
    ac_int<Mant_W+2,true> f_i = f0 * (1 << (Mant_W + 1 - rshift));
    ac_fixed<Mant_W+2,2,true> r;
    r.set_slc(0, f_i);
    return r; 
  }

  inline ac_fixed<53,1,false> frexp_sm_d(double d, ac_int<11,true> &exp, bool &sign) {
    enum {Min_Exp = -1022, Max_Exp = 1023, Mant_W = 52, Denorm_Min_Exp = Min_Exp - Mant_W};
    if(!d) {
      exp = 0;
      sign = false;
      return 0;
    }
    int exp_i;
    bool s = d < 0;
    double f0 = frexp(s ? -d : d, &exp_i);
    AC_ASSERT(exp_i <= Max_Exp+1, "Exponent greater than standard double-precision float exponent max (+1024). It is probably an extended double");
    AC_ASSERT(exp_i >= Denorm_Min_Exp+1, "Exponent less than standard double-precision float exponent min (-1021). It is probably an extended double");
    exp_i--;
    int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : 0;
    exp = exp_i + rshift;
    ac_int<Mant_W+1,false> f_i = f0 * ((Ulong) 1 << (Mant_W + 1 -rshift));
    ac_fixed<Mant_W+1,1,false> r;
    r.set_slc(0, f_i);
    sign = s;
    return r;
  }
  inline ac_fixed<24,1,false> frexp_sm_f(float f, ac_int<8,true> &exp, bool &sign) {
    enum {Min_Exp = -126, Max_Exp = 127, Mant_W = 23, Denorm_Min_Exp = Min_Exp - Mant_W};
    if(!f) {
      exp = 0;
      sign = false;
      return 0;
    }
    int exp_i;
    bool s = f < 0;
    float f0 = frexp(s ? -f : f, &exp_i);
    AC_ASSERT(exp_i <= Max_Exp+1, "Exponent greater than standard single-precision float exponent max (+128). It is probably an extended float");
    AC_ASSERT(exp_i >= Denorm_Min_Exp+1, "Exponent less than standard single-precision float exponent min (-125). It is probably an extended float");
    exp_i--;
    int rshift = exp_i < Min_Exp ? Min_Exp - exp_i : 0;
    exp = exp_i + rshift;
    ac_int<24,false> f_i = f0 * (1 << (Mant_W + 1 - rshift));
    ac_fixed<24,1,false> r;
    r.set_slc(0, f_i);
    sign = s;
    return r;
  }

  */
}


///////////////////////////////////////////////////////////////////////////////

#ifdef __AC_NAMESPACE
}
#endif

#endif // __ALTR_AC_FIXED_H

