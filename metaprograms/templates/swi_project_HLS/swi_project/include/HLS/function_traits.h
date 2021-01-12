#ifndef _INTEL_IHC_HLS_FUNCTION_TRAITS
#define _INTEL_IHC_HLS_FUNCTION_TRAITS

namespace ihc {
  // Some metaprogramming to extract the return type
  // from a function type
  template<typename F>
  struct function_traits {
    using return_type = F;
  };

  template<typename R, typename... Args>
  struct function_traits<R(*)(Args...)>
  {
    using return_type = R;
  };

  template<typename R, typename... Args>
  struct function_traits<R(&)(Args...)>
  {
    using return_type = R;
  };
}

#endif // _INTEL_IHC_HLS_FUNCTION_TRAITS
