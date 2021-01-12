#ifndef _INTEL_IHC_HLS_TASK
#define _INTEL_IHC_HLS_TASK
#include "HLS/function_traits.h"

#include <type_traits>
#include <utility>

// Make sure the macros to take the user calls into implementations
// is not in effect for the source code of the implementation
#undef launch
#undef collect

// Bring in the platform specific '_task' for composition
#if defined(__INTELFPGA_COMPILER__)  && !defined(HLS_X86)
#include "HLS/internal/_task_FPGA.h"
#else
#include "HLS/internal/_task_X64.h"
#endif

namespace ihc {
  namespace internal {
    // The task is a singleton that is shared between
    // a calculation and its result:
    // * Launch the calculations
    //   task<function>::launch(args...);
    // * Get the results
    //   [ret = ]task<function>::collect();
    //      > blocking until launch is finished
    //      > returns result for non-void function
    //
    // When the main program exits, any pending
    // launches will still be processed in their
    // respective thread
    //
    // The task is implemented as an
    // Adaptor Design Pattern
    // The X64 or FPGA implementation is adapted to the desired API
    // This adaptor is straight forward:
    // Both architecture specific implementations are supposed to
    // be interface compatible with this adaptor
    template<typename X, X& f>
    class task {
    public:
      // using F: typename X is different between compilers!
      using F = decltype(f);
      using T = typename ihc::function_traits<F>::return_type;

      // Launch the callable
      template<int capacity, typename ... Args>
      static void launch(Args&& ... args) {
        _t.template launch<capacity>(std::forward<Args>(args)...);
      }

      // Get the result
      template<int capacity>
      static T collect() {
        // Restore void if needed
        return static_cast<T>(_t.template collect<capacity>());
      }

    private:
      // Constructor
      task() {};
      // Destructor
      // Can't be explicit for FPGA target in Intel(R) HLS Compiler
      //~_task() {}

      // Composition (_task is architecture specific)
      // Singleton
      // Anywhere in the function hierarchy where "f" gets called,
      // this specific task (with the corresponding thread and queue)
      // needs to be used
      static internal::_task<X, f> _t;
    }; // class task

    template <typename X, X& f>
    internal::_task<X, f> task<X, f>::_t;

    // Launch (through singleton)
    template<int capacity, class X, X&  f, typename... Args>
    void launch(Args&&... args) {
      task<X, f>::template launch<capacity>(std::forward<Args>(args)...);
    }

    // Collect (through singleton)
    template<int capacity, typename X, X& f>
    typename ihc::function_traits<decltype(f)>::return_type collect() {
      // using F: typename X is different between compilers!
      using F = decltype(f);
      using T = typename ihc::function_traits<F>::return_type;
      // Restore void if needed
      return static_cast<T>(task<X, f>::template collect<capacity>());
    }
  } // namespace internal

  // Fake functions to help with Content Assist (IntelliSense)

  // /!\ Please remember to put parentheses around a launch of
  //     a templated function:
  //     ihc::launch(
  //       ( foo<Types...> ),
  //       args... );
  template<int capacity, typename F, typename... Args>
  void launch(F&& f, Args&&...args);
  template<int capacity, typename F>
  typename ihc::function_traits<F>::return_type collect(F&& f);

} // namespace ihc

// Work around for C++14 (no support for <auto& f>
#define launch(x, ...)  internal::launch <0, decltype(x), x>(__VA_ARGS__)
#define buffered_launch(depth, x, ...)  internal::launch <depth, decltype(x), x>(__VA_ARGS__)

#define collect(x)      internal::collect<0, decltype(x), x>()
#define buffered_collect(depth, x)      internal::collect<depth, decltype(x), x>()

#endif // _INTEL_IHC_HLS_TASK
