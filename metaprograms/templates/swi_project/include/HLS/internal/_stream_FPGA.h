#ifndef _INTEL_IHC_HLS_INTERNAL__STREAM_X64
#define _INTEL_IHC_HLS_INTERNAL__STREAM_X64

namespace ihc {
  namespace internal {
    template<typename T, unsigned char depth = 0>
    class _stream {
    public:
      using type = T;
      bool write(T v, bool blocking = true) {
        if (blocking) {
          __builtin_intel_hls_outstream_write(v, this,
            0, // buffer
            0, // readyLatency
            0, // bitsPerSymbol
            false, // firstSymbolInHighOrderBits
            false, // usesPackets
            false, // usesEmpty
            true,  // usesValid
            true,  // sop
            true,  // eop
            0      // empty
          );
          return true;
        }
        else {
          return   __builtin_intel_hls_outstream_tryWrite(v, this,
            0, // buffer
            0, // readyLatency
            0, // bitsPerSymbol
            false, // firstSymbolInHighOrderBits
            false, // usesPackets
            false, // usesEmpty
            true,  // usesValid
            true,  // sop
            true,  // eop
            0      // empty
          );
        }
      } // write

      bool read(T& v, bool blocking = true) {
        if (blocking) {
          __builtin_intel_hls_instream_read((T*)0, this,
            0, // buffer
            0, // readyLatency
            0, // bitsPerSymbol
            false, // firstSymbolInHighOrderBits
            false, // usesPackets
            false, // usesEmpty
            true,  // usesValid
            true,  // sop
            true,  // eop
            0      // empty
          );
          return true;
        }
        else {
          bool success;
          T* pv = __builtin_intel_hls_instream_tryRead((T*)0, this,
            0, // buffer
            0, // readyLatency
            0, // bitsPerSymbol
            false, // firstSymbolInHighOrderBits
            false, // usesPackets
            false, // usesEmpty
            true,  // usesValid
            true,  // sop
            true,  // eop
            0,     // empty
            &success
          );
          v = *pv;
          return success;
        }
      } // read

    }; // class _stream
  } // namespace internal
} // namespace ihc

#endif // _INTEL_IHC_HLS_INTERNAL__STREAM_X64

