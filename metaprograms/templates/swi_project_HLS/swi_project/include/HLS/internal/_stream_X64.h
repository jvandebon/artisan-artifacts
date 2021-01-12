#ifndef _INTEL_IHC_HLS_INTERNAL__STREAM_X64
#define _INTEL_IHC_HLS_INTERNAL__STREAM_X64
#include <iostream>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace ihc {
  namespace internal {
    template<typename T, unsigned char depth = 0>
    class _stream {
    private:
      // stream is an adaptor of deque
      std::deque<T> _q;
      std::mutex _m;
      std::condition_variable _cv;
    public:
      using type = T;
      bool write(T v, bool blocking = true) {
        std::unique_lock<std::mutex> _{ _m };
        if ((depth != 0) && (_q.size() >= depth)) {
          // Full
          if (!blocking) { return false; }
          // Blocking write
          while (_q.size() >= depth) {
            _cv.wait(_);
          }
        }
        // Unlimited depth, or _q no longer full
        _q.emplace_back(v);
        _.unlock();
        // Notify any blocked read
        _cv.notify_one();
        return true;
      } // write

      bool read(T& v, bool blocking = true) {
        std::unique_lock<std::mutex> _{ _m };
        if (_q.empty()) {
          // Empty
          if (!blocking) { return false; }
          // Blocking read
          while (_q.empty()) {
            _cv.wait(_);
          }
        }
        // _q not, or no longer, empty
        v = _q.front();
        _q.pop_front();
        _.unlock();
        // Notify any blocked write
        _cv.notify_one();
        return true;
      } // read

    }; // class _stream
  } // namespace internal
} // namespace ihc

#endif // _INTEL_IHC_HLS_INTERNAL__STREAM_X64

