#ifndef __LLAE_ARCHIVE_ZLIBCOMMON_H_INCLUDED__
#define __LLAE_ARCHIVE_ZLIBCOMMON_H_INCLUDED__

#include "common.h"

#include <vector>

#include <zlib.h>

namespace archive { namespace impl {

	void pushzerror(lua::state& l,int z_err);

    struct ZLib {
        static constexpr int OK = Z_OK;
        static constexpr int NO_FLUSH = Z_NO_FLUSH;
        static constexpr int STREAM_END = Z_STREAM_END;
        static constexpr int BUF_ERROR = Z_BUF_ERROR;
        static constexpr int FINISH = Z_FINISH;
        using stream = z_stream;
        static void fill_out(stream& z,void* base,size_t len) {
            z.next_out = static_cast<Bytef*>(base);
            z.avail_out = len;
        }
        static void fill_in(stream& z,void* base,size_t len) {
            z.next_in = reinterpret_cast<Bytef*>(base);
            z.avail_in = len;
        }
        static bool has_in(stream& z) {
            return z.avail_in;
        }
        static bool has_out(stream& z) {
            return z.avail_out;
        }
        static void pusherror(lua::state& l,stream& z,int err) {
            if (z.msg) {
                l.pushstring(z.msg);
            } else {
                pushzerror(l,err);
            }
        }
    };
	
    
    
}}

#endif /*__LLAE_ARCHIVE_ZLIBCOMMON_H_INCLUDED__*/
