#pragma once

#include "archive/common.h"
#include "llae-private/zstd.h"

namespace archive{ 

    namespace impl {

        struct ZSTD {
            static constexpr ZSTD_ErrorCode OK = ZSTD_error_no_error;
            static constexpr int NO_FLUSH = 0;
            static constexpr ZSTD_ErrorCode STREAM_END = static_cast<ZSTD_ErrorCode>(ZSTD_error_maxCode + 1);//LZMA_STREAM_END;
            static constexpr ZSTD_ErrorCode BUF_ERROR = ZSTD_error_maxCode;//LZMA_BUF_ERROR;
            static constexpr int FINISH = 0;//LZMA_FINISH;

            using status_t = ZSTD_ErrorCode;
            struct stream {
                ZSTD_DStream* decompress = nullptr;
                ZSTD_outBuffer output;
                ZSTD_inBuffer input;
            };

            static bool alloc(stream& z) {
                z.decompress = ZSTD_createDStream();
                return z.decompress;
            }
            static void dealloc(stream& z) {
                ZSTD_freeDStream(z.decompress);
                z.decompress = nullptr;
            }
            static void fill_out(stream& z,void* base,size_t len) {
                z.output.dst = base;
                z.output.size = len;
                z.output.pos = 0;
            }
            static void fill_in(stream&  z,const void* base,size_t len) {
                z.input.src = base;
                z.input.size = len;
                z.input.pos = 0;
            }
            static bool has_in(stream& z) {
                return z.input.src && z.input.pos != z.input.size;
            }
            static size_t get_avail_out(stream& z) {
                return z.output.size-z.output.pos;
            }
            static void pusherror(lua::state& l,stream& ,ZSTD_ErrorCode err) {
                pusherror(l,err);
            }
            static void pusherror(lua::state& l,ZSTD_ErrorCode err);
        };

        
    }

    class zstduncompress : public impl::compressionstream<zstduncompress,impl::ZSTD> {
        META_OBJECT
    public:
        static ZSTD_ErrorCode process(impl::ZSTD::stream* z,int flush,impl::compressionstream<zstduncompress,impl::ZSTD>&);
        zstduncompress();
        ~zstduncompress();
        size_t init(uv::loop& l);
        
        bool init_decompress(lua::state& l,int argbase);
        
        static lua::multiret decompress(lua::state& l);
        static void lbind(lua::state& l);
    };
    using zstduncompress_ptr = common::intrusive_ptr<zstduncompress>;


    class zstduncompress_read : public impl::compressionstream_read<zstduncompress> {
        META_OBJECT
    public:
        zstduncompress_read();
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using zstduncompress_read_ptr = common::intrusive_ptr<zstduncompress_read>;

   
    class zstduncompress_to_stream : public impl::compressionstream_to_stream<zstduncompress> {
        META_OBJECT
    private:
    public:
        explicit zstduncompress_to_stream( uv::stream_ptr&& stream );
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using zstduncompress_to_stream_ptr = common::intrusive_ptr<zstduncompress_to_stream>;

}

