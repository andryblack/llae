#pragma once

#include "archive/common.h"
#include <lz4.h>
#include <lz4frame.h>

namespace archive{ 

    namespace impl {

        struct LZ4DF {
            static constexpr int OK = 0;
            static constexpr int NO_FLUSH = -1;
            static constexpr int STREAM_END = -2;
            static constexpr int BUF_ERROR = -1000;
            static constexpr int FINISH = -3;

            struct stream {
                LZ4F_dctx* ctx;
                char* next_in;
                char* next_out;
                size_t avail_in;
                size_t avail_out;
                LZ4F_decompressOptions_t options;
            };
            static void fill_out(stream& z,void* base,size_t len) {
                z.next_out = static_cast<char*>(base);
                z.avail_out = len;
            }
            static void fill_in(stream&  z,void* base,size_t len) {
                z.next_in = reinterpret_cast<char*>(base);
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

    class lz4uncompress : public impl::compressionstream<lz4uncompress,impl::LZ4DF> {
        META_OBJECT
    public:
        static int process(impl::LZ4DF::stream* z,int flush,impl::compressionstream<lz4uncompress,impl::LZ4DF>&);
        lz4uncompress();
        ~lz4uncompress();
        int init(uv::loop& l);
        
        bool init_decompress(lua::state& l,int argbase);
        

        //static bool decompress(const void* src,size_t src_size,void* dst,size_t& dst_size);
        static void lbind(lua::state& l);
    };
    using lz4uncompress_ptr = common::intrusive_ptr<lz4uncompress>;


    class lz4uncompress_read : public impl::compressionstream_read<lz4uncompress> {
        META_OBJECT
    public:
        lz4uncompress_read();
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using lz4uncompress_read_ptr = common::intrusive_ptr<lz4uncompress_read>;

   
    class lz4uncompress_to_stream : public impl::compressionstream_to_stream<lz4uncompress> {
        META_OBJECT
    private:
    public:
        explicit lz4uncompress_to_stream( uv::stream_ptr&& stream );
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using lz4uncompress_to_stream_ptr = common::intrusive_ptr<lz4uncompress_to_stream>;

}

