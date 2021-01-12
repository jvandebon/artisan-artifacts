#ifndef __HLS_H__
#define __HLS_H__

#ifdef __INTELFPGA_COMPILER__
   // Compiling for FPGA or x86 using FPGA compiler
#  undef component
#  define component __attribute__((ihc_component)) __attribute__((noinline))
#else
#  ifndef component
#    define component
#  endif
#  ifndef HLS_X86
#    define HLS_X86
#  endif
#endif
#include <type_traits>
#include "HLS/hls_internal.h"
#include "HLS/task.h"
#include "HLS/lsu.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4265) // has virtual functions, but destructor is not virtual
#pragma warning(disable:4505) // unreferenced local function has been removed
#endif

#ifdef __INTELFPGA_COMPILER__
// Memory attributes
#define hls_register                                  __attribute__((__register__))
#define hls_memory                                    __attribute__((__memory__))
#define hls_memory_impl(__x)                          __attribute__((__memory__(__x)))
#define hls_numbanks(__x)                             __attribute__((__numbanks__(__x)))
#define hls_bankwidth(__x)                            __attribute__((__bankwidth__(__x)))
#define hls_singlepump                                __attribute__((__singlepump__))
#define hls_doublepump                                __attribute__((__doublepump__))
#define hls_numports_readonly_writeonly(__rd, __wr)   __attribute__((__numports_readonly_writeonly__(__rd, __wr)))
#define hls_bankbits(__x, ...)                        __attribute__((__bank_bits__(__x, ##__VA_ARGS__)))
#define hls_merge(__x, __y)                           __attribute__((merge(__x, __y)))
#define hls_init_on_reset                             __attribute__((__static_array_reset__(1)))
#define hls_init_on_powerup                           __attribute__((__static_array_reset__(0)))
#define hls_numreadports(__x)                         __attribute__((__numreadports__(__x)))
#define hls_numwriteports(__x)                        __attribute__((__numwriteports__(__x)))
#define hls_simple_dual_port_memory                   __attribute__((simple_dual_port))
#define hls_max_replicates(__x)                       __attribute__((max_replicates(__x)))

// Interface synthesis attributes
#define hls_avalon_streaming_component         __attribute__((component_interface("avalon_streaming")))
#define hls_avalon_slave_component             __attribute__((component_interface("avalon_mm_slave"))) __attribute__((stall_free_return))
#define hls_always_run_component               __attribute__((component_interface("always_run"))) __attribute__((stall_free_return))
#define hls_conduit_argument                   __attribute__((argument_interface("wire")))
#define hls_avalon_slave_register_argument     __attribute__((argument_interface("avalon_mm_slave")))
#define hls_avalon_slave_memory_argument(__x)  __attribute__((local_mem_size(__x))) __attribute__((slave_memory_argument))
#define hls_stable_argument                    __attribute__((stable_argument))
#define hls_stall_free_return                  __attribute__((stall_free_return))

// Component attributes
#define hls_max_concurrency(__x)               __attribute__((max_concurrency(__x)))
#define hls_scheduler_target_fmax_mhz(__x)     __attribute__((scheduler_target_fmax_mhz(__x)))
#define hls_component_ii(__x)                  __attribute__((hls_ii(__x)))
#define hls_disable_component_pipelining       __attribute__((hls_force_loop_pipelining("off")))

// Cluster attributes
#define hls_use_stall_enable_clusters          __attribute__((stall_enable))

// fpga_reg support
#define hls_fpga_reg(__x)                      __fpga_reg(__x)

#else
#define hls_register
#define hls_memory
#define hls_numbanks(__x)
#define hls_bankwidth(__x)
#define hls_singlepump
#define hls_doublepump
#define hls_numports_readonly_writeonly(__rd, __wr)
#define hls_bankbits(__x, ...)
#define hls_merge(__x, __y)
#define hls_init_on_reset
#define hls_init_on_powerup

#define hls_numreadports(__x)
#define hls_numwriteports(__x)

#define hls_simple_dual_port_memory
#define hls_max_replicates(__x)

#define hls_avalon_streaming_component
#define hls_avalon_slave_component
#define hls_always_run_component
#define hls_conduit_argument
#define hls_avalon_slave_register_argument
#define hls_avalon_slave_memory_argument(__x)
#define hls_stable_argument
#define hls_stall_free_return

#define hls_max_concurrency(__x)
#define hls_scheduler_target_fmax_mhz(__x)
#define hls_component_ii(__x)
#define hls_disable_component_pipelining

#define hls_use_stall_enable_clusters

#define hls_fpga_reg(__x) __x
#endif

////////////////////////////////////////////////////////////////////////////////
// Interfaces Declarations
////////////////////////////////////////////////////////////////////////////////

namespace ihc {

  ////////////////////////////////
 /// memory master interface  ///
////////////////////////////////

  template<int _N> struct dwidth {
    static constexpr int value = _N;
    static constexpr int defaultValue = 64;
  };

  template<int _N> struct awidth {
    static constexpr int value = _N;
    static constexpr int defaultValue = 64;
  };

  template<int _N> struct latency {
    static constexpr int value = _N;
    static constexpr int defaultValue = 1;
  };

  template<int _N> struct readwrite_mode {
    // Should be enum readwrite_t but we don't know how to make GetValue generic
    static constexpr enum readwrite_t value = (readwrite_t) _N;
    static constexpr enum readwrite_t defaultValue = readwrite;
  };

  template<int _N> struct maxburst {
    static constexpr int value = _N;
    static constexpr int defaultValue = 1;
  };

  template<int _N> struct align {
    static constexpr int value = _N;
    static constexpr int defaultValue = -1;
  };

  template<int _N> struct aspace {
    static constexpr int value = _N;
    static constexpr int defaultValue = 1;
  };

  template<int _N> struct waitrequest {
    static constexpr int value = _N;
    static constexpr int defaultValue = false;
  };

  template <template <int> class _Type, class _T>
  struct MatchType : std::is_same<_Type<_T::value>,_T> {};

  template <template <int> class _Type, class ... _T>
  struct GetValue {
    // any value is ok here, so '0' is fine for an arbitrary instantiation
    enum { value = _Type<0>::defaultValue };
    // only when _T is empty
  };

  template <template <int> class _Type, class _T1, class ... _T>
  struct GetValue<_Type, _T1, _T...> {
    enum { value = std::conditional<MatchType<_Type, _T1>::value, _T1, GetValue<_Type, _T...>>::type::value };
  };

template <typename _DT, class ... _Params>
class mm_master final
#ifdef HLS_X86
  : public internal::memory_base
#endif
{
public:

#ifdef HLS_X86
  template <typename _T>
  explicit mm_master(_T *data, std::size_t size = 0, bool use_socket = false)
      : internal::memory_base(_aspace, _awidth, _dwidth, _latency,
                              _readwrite_mode, true, _maxburst, _align,
                              _waitrequest, data, size, sizeof(_DT),
                              use_socket) {
    mSize = size;
    mUse_socket = use_socket;
    if (size > 0 && size % sizeof(_DT) != 0) {
      __ihc_hls_runtime_error_x86(
          "The buffer size must be a multiple of the type size");
    }
  }
#else
  template<typename _T> explicit mm_master(_T *data, std::size_t size=0, bool use_socket=false);
#endif

  // The copy constructor and assignment operator are needed in the testbench
  // but illegal in a component
  mm_master(const mm_master &other);

  mm_master& operator=(const mm_master& other);

  // Clean up any derrived mm_masters when this object is destroyed.
  ~mm_master();

  //////////////////////////////////////////////////////////////////////////////
  // The following operators apply to the mm_master object and are only
  // supported in the testbench:
  //   mm_master()
  //   getInterfaceAtIndex()
  //////////////////////////////////////////////////////////////////////////////
  // The following operators apply to the base pointer and should only be used
  // in the component:
  //   operator[]()
  //   operator*()
  //   operator->()
  //   operator _T()
  //   operator+()
  //   operator&()
  //   operator|()
  //   operator^()
  //////////////////////////////////////////////////////////////////////////////

  _DT &operator[](int index);
  _DT &operator*();
  _DT *operator->();
  template<typename _T> operator _T();
  _DT *operator+(int index);
  template<typename _T> _DT *operator&(_T value);
  template<typename _T> _DT *operator|(_T value);
  template<typename _T> _DT *operator^(_T value);
  // This function is only supported in the testbench:
  mm_master<_DT, _Params...>& getInterfaceAtIndex(int index);

#ifdef HLS_X86
private:
  std::vector<internal::memory_base* > new_masters;
#else //Fpga


#endif
private:
  static constexpr int _dwidth   = GetValue<ihc::dwidth, _Params...>::value;
  static constexpr int _awidth   = GetValue<ihc::awidth, _Params...>::value;
  static constexpr int _aspace   = GetValue<ihc::aspace, _Params...>::value;
  static constexpr int _latency  = GetValue<ihc::latency, _Params...>::value;
  static constexpr int _maxburst = GetValue<ihc::maxburst, _Params...>::value;
  static constexpr int _align    = (GetValue<ihc::align, _Params...>::value == -1) ? alignof(_DT) : GetValue<ihc::align, _Params...>::value;
  static constexpr int _readwrite_mode = GetValue<ihc::readwrite_mode, _Params...>::value;
  static constexpr bool _waitrequest = GetValue<ihc::waitrequest, _Params...>::value;

  _DT __hls_mm_master_aspace(_aspace) *mPtr;
  int mSize;
  bool mUse_socket;
};
  /////////////////////////////
 /// streaming interfaces  ///
//////////////////////////////

  template<int _N> struct buffer {
    static constexpr int value = _N;
    static constexpr int defaultValue = 0;
  };

  template<int _N> struct readyLatency {
    static constexpr int value = _N;
    static constexpr int defaultValue = 0;
  };

  template<int _N> struct bitsPerSymbol {
    static constexpr int value = _N;
    static constexpr int defaultValue = 0;
  };

  template<int _N> struct usesPackets {
    static constexpr bool value = _N;
    static constexpr bool defaultValue = false;
  };

  template<int _N> struct usesValid {
    static constexpr int value = _N;
    static constexpr int defaultValue = true;
  };

  template<int _N> struct usesReady {
    static constexpr int value = _N;
    static constexpr int defaultValue = true;
  };

  template<int _N> struct usesEmpty {
    static constexpr int value = _N;
    static constexpr int defaultValue = false;
  };

  template<int _N> struct firstSymbolInHighOrderBits {
    static constexpr int value = _N;
    static constexpr int defaultValue = false;
  };

template <typename _T, class ... _Params>
class stream_in final : public internal::stream<_T, _Params...> {
public:
  stream_in();
  stream_in(const stream_in&) = delete;
  stream_in(const stream_in&&) = delete;
  stream_in& operator=(const stream_in&) = delete;
  stream_in& operator=(const stream_in&&) = delete;
  _T read(bool wait=false);
  void write(const _T& arg);
  _T tryRead(bool &success);
  bool tryWrite(const _T& arg);

  // for packet based stream
  _T read(bool& sop, bool& eop, bool wait=false);
  _T read(bool& sop, bool& eop, int& empty, bool wait=false);
  void write(const _T& arg, bool sop, bool eop);
  void write(const _T& arg, bool sop, bool eop, int empty);
  _T tryRead(bool &success, bool& sop, bool& eop);
  _T tryRead(bool &success, bool& sop, bool& eop, int& empty);
  bool tryWrite(const _T& arg, bool sop, bool eop);
  bool tryWrite(const _T& arg, bool sop, bool eop, int empty);
  void setStallCycles(unsigned average_stall, unsigned stall_delta=0);
  void setValidCycles(unsigned average_valid, unsigned valid_delta=0);

 private:
    static constexpr int _buffer   = GetValue<ihc::buffer, _Params...>::value;
    static constexpr int _readyLatency   = GetValue<ihc::readyLatency, _Params...>::value;
    static constexpr int _bitsPerSymbol  = GetValue<ihc::bitsPerSymbol, _Params...>::value;
    static constexpr bool _firstSymbolInHighOrderBits =  GetValue<ihc::firstSymbolInHighOrderBits, _Params...>::value;
    static constexpr bool _usesPackets  = GetValue<ihc::usesPackets, _Params...>::value;
    static constexpr bool _usesEmpty = GetValue<ihc::usesEmpty, _Params...>::value;
    static constexpr bool _usesValid = GetValue<ihc::usesValid, _Params...>::value;
    static constexpr bool _usesReady = GetValue<ihc::usesReady, _Params...>::value;
};

template <typename _T, class ... _Params>
class stream_out final : public internal::stream<_T, _Params...> {

public:
  stream_out();
  stream_out(const stream_out&) = delete;
  stream_out(const stream_out&&) = delete;
  stream_out& operator=(const stream_out&) = delete;
  stream_out& operator=(const stream_out&&) = delete;
  _T read(bool wait=false);
  void write(const _T& arg);
  _T tryRead(bool &success);
  bool tryWrite(const _T& arg);

  // for packet based stream
  _T read(bool& sop, bool& eop, bool wait=false);
  _T read(bool& sop, bool& eop, int& empty, bool wait=false);
  void write(const _T& arg, bool sop, bool eop);
  void write(const _T& arg, bool sop, bool eop, int empty);
  _T tryRead(bool &success, bool& sop, bool& eop);
  _T tryRead(bool &success, bool& sop, bool& eop, int& empty);
  bool tryWrite(const _T& arg, bool sop, bool eop);
  bool tryWrite(const _T& arg, bool sop, bool eop, int empty);
  void setStallCycles(unsigned average_stall, unsigned stall_delta=0);
  void setReadyCycles(unsigned average_ready, unsigned ready_delta=0);

 private:
    static constexpr int _buffer   = GetValue<ihc::buffer, _Params...>::value;
    static constexpr int _readyLatency   = GetValue<ihc::readyLatency, _Params...>::value;
    static constexpr int _bitsPerSymbol  = GetValue<ihc::bitsPerSymbol, _Params...>::value;
    static constexpr bool _firstSymbolInHighOrderBits = GetValue<ihc::firstSymbolInHighOrderBits, _Params...>::value;
    static constexpr bool _usesPackets  = GetValue<ihc::usesPackets, _Params...>::value;
    static constexpr bool _usesEmpty = GetValue<ihc::usesEmpty, _Params...>::value;
    static constexpr bool _usesValid = GetValue<ihc::usesValid, _Params...>::value;
    static constexpr bool _usesReady = GetValue<ihc::usesReady, _Params...>::value;
};


// Bi-directional inter-task stream
template<typename _T, class ... _Params>
class stream final : public internal::stream<_T, _Params...> {
public:
  stream();
  stream(const stream&) = delete;
  stream(const stream&&) = delete;
  stream& operator=(const stream&) = delete;
  stream& operator=(const stream&&) = delete;
  _T read(bool wait=true);
  void write(const _T& arg);
  _T tryRead(bool &success);
  bool tryWrite(const _T& arg);

  // for packet based stream
  _T read(bool& sop, bool& eop, bool wait=true);
  _T read(bool& sop, bool& eop, int& empty, bool wait=true);
  void write(const _T& arg, bool sop, bool eop);
  void write(const _T& arg, bool sop, bool eop, int empty);
  _T tryRead(bool &success, bool& sop, bool& eop);
  _T tryRead(bool &success, bool& sop, bool& eop, int& empty);
  bool tryWrite(const _T& arg, bool sop, bool eop);
  bool tryWrite(const _T& arg, bool sop, bool eop, int empty);

private:
  static constexpr int _buffer   = GetValue<ihc::buffer, _Params...>::value;
  static constexpr int _readyLatency   = GetValue<ihc::readyLatency, _Params...>::value;
  static constexpr int _bitsPerSymbol  = GetValue<ihc::bitsPerSymbol, _Params...>::value;
  static constexpr bool _firstSymbolInHighOrderBits = GetValue<ihc::firstSymbolInHighOrderBits, _Params...>::value;
  static constexpr bool _usesPackets  = GetValue<ihc::usesPackets, _Params...>::value;
  static constexpr bool _usesEmpty = GetValue<ihc::usesEmpty, _Params...>::value;
  static constexpr bool _usesValid = GetValue<ihc::usesValid, _Params...>::value;
  static constexpr bool _usesReady = GetValue<ihc::usesReady, _Params...>::value;
  static_assert(_usesValid, "Bi-directional stream interfaces must use Valid signal");
  static_assert(_usesReady, "Bi-directional stream interfaces must use Ready signal");

};

}//namespace ihc

////////////////////////////////////////////////////////////////////////////////
// HLS Cosimulation Support API
////////////////////////////////////////////////////////////////////////////////

#define ihc_hls_enqueue(retptr, func, ...) \
  { \
    if (__ihc_hls_async_call_capable()){ \
      __ihc_enqueue_handle=(retptr); \
      (void) (*(func))(__VA_ARGS__); \
      __ihc_enqueue_handle=0; \
    } else { \
      *(retptr) = (*(func))(__VA_ARGS__); \
    } \
  }

#define ihc_hls_enqueue_noret(func, ...) \
  { \
  __ihc_enqueue_handle=& __ihc_enqueue_handle; \
  (*(func))(__VA_ARGS__); \
  __ihc_enqueue_handle=0; \
  }

#define ihc_hls_component_run_all(component_address) \
  __ihc_hls_component_run_all((void*) (component_address))

#define ihc_hls_set_component_wait_cycle(component_address, num_wait_cycles) \
  __ihc_hls_set_component_wait_cycle((void*) (component_address), num_wait_cycles)

// When running a simulation, this function will issue a reset to all components
// in the testbench
// Returns: 0 if reset did not occur (ie. if the component target is x86)
//          1 if reset occured (ie. if the component target is an FPGA)
extern "C" int ihc_hls_sim_reset(void);

////////////////////////////////////////////////////////////////////////////////
// HLS Component Built-Ins
////////////////////////////////////////////////////////////////////////////////

//Builtin memory fence function call
#ifdef HLS_X86
inline void ihc_fence() {}

#else
extern "C" void __acl_mem_fence(unsigned int);
inline void ihc_fence() {
  // fence on all types of fences from OpenCL
  __acl_mem_fence(-1);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Implementions, no declarations below
////////////////////////////////////////////////////////////////////////////////

namespace ihc {
#ifdef HLS_X86

  //////////////////
 /// mm_master  ///
//////////////////

  // The copy constructor and assignment operator are needed in the testbench
  // necessary to ensurebut illegal in a component
template <typename _DT, class... _Params>
mm_master<_DT, _Params...>::mm_master(const mm_master &other)
    : internal::memory_base(_aspace, _awidth, _dwidth, _latency,
                            static_cast<readwrite_t>(_readwrite_mode), true,
                            _maxburst, _align, _waitrequest, other.get_base(),
                            other.get_size(), sizeof(_DT),
                            other.uses_socket()) {
  mPtr = other.mPtr;
  mSize = other.mSize;
  mUse_socket = other.mUse_socket;
  mem = other.mem;
}

template <typename _DT, class ... _Params>
  mm_master<_DT, _Params...>& mm_master<_DT, _Params...>::operator=(const mm_master& other) {
    mPtr = other.mPtr;
    mSize = other.mSize;
    mUse_socket = other.m_Use_socket;
    mem = other.mem;
  }

  // Clean up any derrived mm_masters when this object is destroyed.
template <typename _DT, class ... _Params>
  mm_master<_DT, _Params...>::~mm_master() {
    for(std::vector<internal::memory_base* >::iterator it = new_masters.begin(),
        ie = new_masters.end(); it != ie; it++) {
      delete *it;
    }
    new_masters.clear();
  }

template <typename _DT, class ... _Params>
_DT &mm_master<_DT, _Params... >::operator[](int index) {
  assert(size==0 || index*data_size<size);
  return ((_DT*)mem)[index];
}

template <typename _DT, class ... _Params>
_DT &mm_master<_DT, _Params...>::operator*() {
  return ((_DT*)mem)[0];
}

template <typename _DT, class ... _Params>
_DT *mm_master<_DT, _Params...>::operator->() {
  return (_DT*)mem;
}

template <typename _DT, class ... _Params>
template<typename _T> mm_master<_DT, _Params...>::operator _T() {
  return (_T)((unsigned long long)mem);
}

template <typename _DT, class ... _Params>
_DT *mm_master<_DT, _Params...>::operator+(int index) {
  assert(size==0 || index*data_size<size);
  return &((_DT*)mem)[index];
}

// Bitwise operators
template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator&(_T value) {
  return (_DT*)((unsigned long long)mem & (unsigned long long)value);
}

template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator|(_T value) {
  return (_DT*)((unsigned long long)mem | (unsigned long long)value);
}

template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator^(_T value) {
  return (_DT*)((unsigned long long)mem ^ (unsigned long long)value);
}

// Function for creating new mm_master at an offset
template <typename _DT, class ... _Params>
mm_master<_DT, _Params...>& mm_master<_DT,_Params...>::getInterfaceAtIndex(int index) {
  assert(mSize==0 || index*data_size<mSize);
  // This new object is cleaned up when this' destructor is called.
  mm_master<_DT, _Params...> *temp = new mm_master(&(((_DT*)mem)[index]), mSize - index * sizeof(_DT), mUse_socket);
  new_masters.push_back(temp);
  return *temp;
}

  ///////////////////
 /// stream_in   ///
///////////////////

template<typename _T, class ... _Params>
stream_in<_T,_Params...>::stream_in() {}

template<typename _T, class ... _Params>
  _T stream_in<_T, _Params...>::tryRead(bool &success) {
  return internal::stream<_T,_Params...>::tryRead(success);
}

template<typename _T, class ... _Params>
  _T stream_in<_T,_Params...>::read(bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(wait);
    return elem;
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg);
  }
  return success;
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg) {
    internal::stream<_T,_Params...>::write(arg);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop, empty);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::read(bool& sop, bool& eop, bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, wait);
    return elem;
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, empty, wait);
    return elem;
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop);
  }
  return success;
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop, empty);
  }
  return success;
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
    internal::stream<_T,_Params...>::write(arg, sop, eop);
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
    internal::stream<_T,_Params...>::write(arg, sop, eop, empty);
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::setStallCycles(unsigned average_stall, unsigned stall_delta) {
  if (stall_delta > average_stall) {
    __ihc_hls_runtime_error_x86("The stall delta in setStallCycles cannot be larger than the average stall value");
  }
  internal::stream<_T,_Params...>::setStallCycles(average_stall, stall_delta);
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::setValidCycles(unsigned average_valid, unsigned valid_delta) {
  if (average_valid == 0) {
    __ihc_hls_runtime_error_x86("The valid average in setValidCycles must be at least 1");
  }
  if (valid_delta > average_valid) {
    __ihc_hls_runtime_error_x86("The valid delta in setValidCycles cannot be larger than the average valid value");
  }
  internal::stream<_T,_Params...>::setReadyorValidCycles(average_valid, valid_delta);
}

  ///////////////////
 /// stream_out  ///
///////////////////

template<typename _T, class ... _Params>
  stream_out<_T,_Params...>::stream_out() {
}

template<typename _T, class ... _Params>
  _T stream_out<_T,_Params...>::tryRead(bool &success) {
  return internal::stream<_T,_Params...>::tryRead(success);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(wait);
    return elem;
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg) {
    internal::stream<_T,_Params...>::write(arg);
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg);
  }
  return success;
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop, empty);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool& sop, bool& eop, bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, wait);
    return elem;
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait /*=false*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, empty, wait);
    return elem;
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
    internal::stream<_T,_Params...>::write(arg, sop, eop);
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
  internal::stream<_T,_Params...>::write(arg, sop, eop, empty);
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop);
  }
  return success;
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop, empty);
  }
  return success;
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::setStallCycles(unsigned average_stall, unsigned stall_delta) {
  if (stall_delta > average_stall) {
    __ihc_hls_runtime_error_x86("The stall delta in setStallCycles cannot be larger than the average stall value");
  }
  internal::stream<_T,_Params...>::setStallCycles(average_stall, stall_delta);
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::setReadyCycles(unsigned average_ready, unsigned ready_delta) {
  if (average_ready == 0) {
    __ihc_hls_runtime_error_x86("The ready average in setReadCycles must be at least 1");
  }
  if (ready_delta > average_ready) {
    __ihc_hls_runtime_error_x86("The ready delta in setReadyCycles cannot be larger than the average ready value");
  }
  internal::stream<_T,_Params...>::setReadyorValidCycles(average_ready, ready_delta);
}

  ///////////////////
  ///// stream  /////
  ///////////////////

template<typename _T, class ... _Params>
  stream<_T,_Params...>::stream() {
}

template<typename _T, class ... _Params>
  _T stream<_T,_Params...>::tryRead(bool &success) {
  return internal::stream<_T,_Params...>::tryRead(success);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool wait /*=true*/) {
    _T elem = internal::stream<_T,_Params...>::read(wait);
    return elem;
}

template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg) {
    internal::stream<_T,_Params...>::write(arg);
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg);
  }
  return success;
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  return internal::stream<_T,_Params...>::tryRead(success, sop, eop, empty);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool& sop, bool& eop, bool wait /*=true*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, wait);
    return elem;
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait /*=true*/) {
    _T elem = internal::stream<_T,_Params...>::read(sop, eop, empty, wait);
    return elem;
}

template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
    internal::stream<_T,_Params...>::write(arg, sop, eop);
}

template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
  internal::stream<_T,_Params...>::write(arg, sop, eop, empty);
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop);
  }
  return success;
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  bool success = true; /* stl::queue has no full */
  if (success) {
    write(arg, sop, eop, empty);
  }
  return success;
}


#else //fpga path. Ignore the class just return a consistant pointer/reference

  //////////////////
 /// mm_master  ///
//////////////////
template <typename _DT, class ... _Params>
_DT &mm_master<_DT, _Params...>::operator[](int index) {
  return *(_DT*)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, index);
}

template <typename _DT, class ... _Params>
_DT &mm_master<_DT,_Params...>::operator*(){
  return *(_DT*)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0);
}

template <typename _DT, class ... _Params>
_DT *mm_master<_DT,_Params...>::operator->(){
  return (_DT*)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0);
}

template <typename _DT, class ... _Params>
_DT *mm_master<_DT, _Params...>::operator+(int index) {
  return (_DT*)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0) + index;
}

template <typename _DT, class ... _Params>
template<typename _T> mm_master<_DT, _Params...>::operator _T() {
  return (_T)((unsigned long long)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0));
}

// Bitwise operators
template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator&(_T value) {
  return (_DT*)(((unsigned long long)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0)) & (unsigned long long)value);
}

template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator|(_T value) {
  return (_DT*)(((unsigned long long)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0)) | (unsigned long long)value);
}

template <typename _DT, class ... _Params>
template<typename _T> _DT *mm_master<_DT, _Params...>::operator^(_T value) {
  return (_DT*)(((unsigned long long)__builtin_intel_hls_mm_master_load(mPtr, mSize, mUse_socket, _dwidth, _awidth, _aspace, _latency, _maxburst, _align, _readwrite_mode, _waitrequest, (int)0)) ^ (unsigned long long)value);
}

  ///////////////////
 /// stream_in   ///
///////////////////

template<typename _T, class ... _Params>
  _T stream_in<_T, _Params...>::tryRead(bool &success) {
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &emp, &success);
}
template<typename _T, class ... _Params>
  _T stream_in<_T,_Params...>::read(bool wait) {
  (void)wait;
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return  *__builtin_intel_hls_instream_read((_T*)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &emp);
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg) {
 __builtin_intel_hls_instream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, false, false, 0 );
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg) {
  return  __builtin_intel_hls_instream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, false, false, 0);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  int emp = 0;
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer,  _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &emp, &success);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based reads require a stream with the parameterization: usesEmpty<true>");
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &empty, &success);
}

template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::read(bool& sop, bool& eop, bool wait) {
  (void)wait;
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  int emp = 0;
  return  *__builtin_intel_hls_instream_read((_T*)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &emp);
}
template<typename _T, class ... _Params>
_T stream_in<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait) {
  (void) wait;
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based reads require a stream with the parameterization: usesEmpty<true>");
  return  *__builtin_intel_hls_instream_read((_T*)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, &sop, &eop, &empty);
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  return  __builtin_intel_hls_instream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid,  sop, eop, 0);
}

template<typename _T, class ... _Params>
bool stream_in<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  return  __builtin_intel_hls_instream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid, sop, eop, empty);
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
 __builtin_intel_hls_instream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid,  sop, eop, 0 );
}

template<typename _T, class ... _Params>
void stream_in<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
 __builtin_intel_hls_instream_write(&arg, (__int64)this,  _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesValid,  sop, eop, empty );
}

  ///////////////////
 /// stream_out  ///
///////////////////

template<typename _T, class ... _Params>
  _T stream_out<_T,_Params...>::tryRead(bool &success) {
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return *__builtin_intel_hls_outstream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp, &success);
}
template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool wait) {
  (void)wait;
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return *__builtin_intel_hls_outstream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp );
}
template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg) {
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
 __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, false, false, 0);
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg) {
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, false, false, 0);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  int emp = 0;
  return *__builtin_intel_hls_outstream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp, &success);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  return *__builtin_intel_hls_outstream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  &sop, &eop, &empty, &success);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool& sop, bool& eop, bool wait) {
  (void)wait;
  int emp;
  return *__builtin_intel_hls_outstream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp);
}

template<typename _T, class ... _Params>
_T stream_out<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait) {
  (void)wait;
  return *__builtin_intel_hls_outstream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &empty );
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
    static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, sop, eop, 0);
}

template<typename _T, class ... _Params>
void stream_out<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based writes require a stream with the parameterization: usesEmpty<true>");
  __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  sop, eop, empty);
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, sop, eop, 0);
}

template<typename _T, class ... _Params>
bool stream_out<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based writes require a stream with the parameterization: usesEmpty<true>");

  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  sop, eop, empty);
}

  ///////////////////
  ///// stream  /////
  ///////////////////

template<typename _T, class ... _Params>
  _T stream<_T,_Params...>::tryRead(bool &success) {
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp, &success);
}
template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool wait) {
  (void)wait;
  bool sop = false;
  bool eop = false;
  int emp = 0;
  return *__builtin_intel_hls_instream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp );
}
template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg) {
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
 __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, false, false, 0);
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg) {
  static_assert(_usesPackets || !_usesEmpty, "Empty based reads require a stream with the parametrizations: usesPackets<true>, usesEmpty<true>");
  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, false, false, 0);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop) {
  int emp = 0;
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp, &success);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::tryRead(bool &success, bool& sop, bool& eop, int& empty) {
  return *__builtin_intel_hls_instream_tryRead((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  &sop, &eop, &empty, &success);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool& sop, bool& eop, bool wait) {
  (void)wait;
  int emp;
  return *__builtin_intel_hls_instream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &emp);
}

template<typename _T, class ... _Params>
_T stream<_T,_Params...>::read(bool& sop, bool& eop, int& empty, bool wait) {
  (void)wait;
  return *__builtin_intel_hls_instream_read((_T *)0, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, &sop, &eop, &empty );
}

template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg, bool sop, bool eop) {
    static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, sop, eop, 0);
}

template<typename _T, class ... _Params>
void stream<_T,_Params...>::write(const _T& arg, bool sop, bool eop, int empty) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based writes require a stream with the parameterization: usesEmpty<true>");
  __builtin_intel_hls_outstream_write(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  sop, eop, empty);
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady, sop, eop, 0);
}

template<typename _T, class ... _Params>
bool stream<_T,_Params...>::tryWrite(const _T& arg, bool sop, bool eop, int empty) {
  static_assert(_usesPackets, "Using start_of_packet and end_of_packet requires a stream with the parameterization: usesPackets<true>");
  static_assert(_usesEmpty, "Empty based writes require a stream with the parameterization: usesEmpty<true>");

  return __builtin_intel_hls_outstream_tryWrite(&arg, (__int64)this, _buffer, _readyLatency, _bitsPerSymbol, _firstSymbolInHighOrderBits, _usesPackets, _usesEmpty, _usesReady,  sop, eop, empty);
}

#endif
} // namespace ihc

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
