#ifndef __LLAE_ARCHIVE_BZIP2_COMMON_H_INCLUDED__
#define __LLAE_ARCHIVE_BZIP2_COMMON_H_INCLUDED__

#include "archive/common.h"
#include "llae-private/bzlib.h"

namespace archive{ namespace impl {

    struct BZLib {
        static constexpr int OK = BZ_OK;
        static constexpr int NO_FLUSH = 0;
        static constexpr int STREAM_END = BZ_STREAM_END;
        static constexpr int BUF_ERROR = -1000;
        static constexpr int FINISH = BZ_FINISH;
        using status_t = int;
        using stream = bz_stream;
        static void alloc(stream& z) {
            memset(&z, 0, sizeof(z));
        }
        static void dealloc(stream&) {}
        static void fill_out(stream& z,void* base,size_t len) {
            z.next_out = static_cast<char*>(base);
            z.avail_out = static_cast<unsigned int>(len);
        }
        static void fill_in(stream& z,const void* base,size_t len) {
            z.next_in = reinterpret_cast<char*>(const_cast<void*>(base));
            z.avail_in = static_cast<unsigned int>(len);
        }
        static bool has_in(stream& z) {
            return z.avail_in;
        }
        static size_t get_avail_out(stream& z) {
            return z.avail_out;
        }
        static void pusherror(lua::state& l,stream& z,int err);
    };

    
    
}}

#endif /*__LLAE_ARCHIVE_BZIP2_COMMON_H_INCLUDED__*/
