#ifndef __LLAE_ARCHIVE_BZIP2_UNCOMPRESS_H_INCLUDED__
#define __LLAE_ARCHIVE_BZIP2_UNCOMPRESS_H_INCLUDED__

#include "common.h"

namespace archive {

    class bzlibuncompress : public impl::compressionstream<bzlibuncompress,impl::BZLib> {
        META_OBJECT
    public:
        static int process(bz_stream* z,int flush,impl::compressionstream<bzlibuncompress,impl::BZLib>&);
        bzlibuncompress();
        ~bzlibuncompress();
        int init(uv::loop& l,int small);
        bool is_error() const { return m_is_error; }
        
        bool init_decompress(lua::state& l,int argbase);
        

        static bool decompress(const void* src,size_t src_size,void* dst,size_t& dst_size);
        static void lbind(lua::state& l);
    };
    using bzlibuncompress_ptr = common::intrusive_ptr<bzlibuncompress>;

    class bzlibuncompress_read : public impl::compressionstream_read<bzlibuncompress> {
        META_OBJECT
    public:
        bzlibuncompress_read();
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using bzlibuncompress_read_ptr = common::intrusive_ptr<bzlibuncompress_read>;

   
    class bzlibuncompress_to_stream : public impl::compressionstream_to_stream<bzlibuncompress> {
        META_OBJECT
    private:
    public:
        explicit bzlibuncompress_to_stream( uv::stream_ptr&& stream );
        static void lbind(lua::state& l);
        static lua::multiret new_decompress(lua::state& l);
    };
    using bzlibuncompress_to_stream_ptr = common::intrusive_ptr<bzlibuncompress_to_stream>;



}

#endif /*__LLAE_ARCHIVE_BZIP2_UNCOMPRESS_H_INCLUDED__*/
