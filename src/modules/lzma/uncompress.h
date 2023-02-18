#pragma once

#include "archive/common.h"
#include <lzma.h>

namespace archive{ 

    namespace impl {

        struct LZMA {
            static constexpr int OK = LZMA_OK;
            static constexpr int NO_FLUSH = 0;
            static constexpr int STREAM_END = LZMA_STREAM_END;
            static constexpr int BUF_ERROR = LZMA_BUF_ERROR;
            static constexpr int FINISH = LZMA_FINISH;

            using stream = lzma_stream;

            static void fill_out(stream& z,void* base,size_t len) {
                z.next_out = static_cast<uint8_t*>(base);
                z.avail_out = len;
            }
            static void fill_in(stream&  z,void* base,size_t len) {
                z.next_in = reinterpret_cast<uint8_t*>(base);
                z.avail_in = len;
            }
            static bool has_in(stream& z) {
                return z.avail_in;
            }
            static bool has_out(stream& z) {
                return z.avail_out;
            }
            static void pusherror(lua::state& l,stream& z,int err);
        };

        
    }

    class lzmauncompress : public impl::compressionstream<lzmauncompress,impl::LZMA> {
        META_OBJECT
    public:
        static int process(impl::LZMA::stream* z,int flush,impl::compressionstream<lzmauncompress,impl::LZMA>&);
        lzmauncompress();
        ~lzmauncompress();
        int init(uv::loop& l);
        
        bool init_decompress(lua::state& l,int argbase);
        
        static lua::multiret decompress(lua::state& l);
        static lua::multiret decompress_params(lua::state& l);
        static void lbind(lua::state& l);
    };
    using lzmauncompress_ptr = common::intrusive_ptr<lzmauncompress>;


    class lzmauncompress_read : public impl::compressionstream_read<lzmauncompress> {
        META_OBJECT
    public:
        lzmauncompress_read();
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using lzmauncompress_read_ptr = common::intrusive_ptr<lzmauncompress_read>;

   
    class lzmauncompress_to_stream : public impl::compressionstream_to_stream<lzmauncompress> {
        META_OBJECT
    private:
    public:
        explicit lzmauncompress_to_stream( uv::stream_ptr&& stream );
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using lzmauncompress_to_stream_ptr = common::intrusive_ptr<lzmauncompress_to_stream>;

}

