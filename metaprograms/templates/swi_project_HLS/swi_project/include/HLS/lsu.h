/**************************************************************************
 *                                                                        *
 *  A library that allows controlling the LSU of a given load/store site  *
 *  Copyright 2019, Intel Corporation                                     *
 *  All Rights Reserved.                                                  *
 *                                                                        *
 *  Author: Mohammad Fawaz                                                *
 *                                                                        *
 **************************************************************************/

#ifndef __HLS_LSU_H__
#define __HLS_LSU_H__

namespace ihc {
// Forward declarations. These are defined in HLS/hls.h
template <typename _DT, class... _Params> class mm_master;
template <template <int> class _Type, class... _T> struct GetValue;

enum style_t {
  BURST_COALESCED = 0,
  PIPELINED,
};

template <int _N> struct style {
  static constexpr enum style_t value = (style_t)_N;
  static constexpr enum style_t defaultValue = BURST_COALESCED;
};

template <int _N> struct static_coalescing {
  static constexpr int value = _N;
  static constexpr int defaultValue = 1;
};

template <class... _LSUParams> class lsu final {
public:
  lsu() = delete;

#ifdef HLS_X86
  template <class _DT, class... _MMParams>
  static _DT &load(mm_master<_DT, _MMParams...> &Ptr) {
    return *Ptr;
  }

  template <class _DT> static _DT &load(_DT *Ptr) { return *Ptr; }

  template <class _DT, class... _MMParams>
  static void store(mm_master<_DT, _MMParams...> &Ptr, _DT Val) {
    *Ptr = Val;
  }

  template <class _DT> static void store(_DT *Ptr, _DT Val) { *Ptr = Val; }
#else
  template <class _DT, class... _MMParams>
  static _DT &load(mm_master<_DT, _MMParams...> &Ptr) {
    CheckLoad();
    return *__builtin_fpga_mem(&*Ptr, _style == BURST_COALESCED, 0, 0, 0, 0,
                               _static_coalescing);
  }

  template <class _DT> static _DT &load(_DT *Ptr) {
    CheckLoad();
    return *__builtin_fpga_mem(Ptr, _style == BURST_COALESCED, 0, 0, 0, 0,
                               _static_coalescing);
  }

  template <class _DT, class... _MMParams>
  static void store(mm_master<_DT, _MMParams...> &Ptr, _DT Val) {
    CheckStore();
    *__builtin_fpga_mem(&*Ptr, _style == BURST_COALESCED, 0, 0, 0, 0,
                        _static_coalescing) = Val;
  }

  template <class _DT> static void store(_DT *Ptr, _DT Val) {
    CheckStore();
    *__builtin_fpga_mem(Ptr, _style == BURST_COALESCED, 0, 0, 0, 0,
                        _static_coalescing) = Val;
  }
#endif

private:
  static constexpr int _style = GetValue<ihc::style, _LSUParams...>::value;
  static constexpr int _static_coalescing =
      GetValue<ihc::static_coalescing, _LSUParams...>::value;

  static void CheckLoad() {
    static_assert(_style == BURST_COALESCED || _style == PIPELINED,
                  "Invalid LSU style choice for a load operation");
  }
  static void CheckStore() {
    static_assert(_style == BURST_COALESCED || _style == PIPELINED,
                  "Invalid LSU style choice for a store operation");
  }
};
} // namespace ihc

#endif //__HLS_LSU_H__
