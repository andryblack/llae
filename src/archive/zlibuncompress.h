#ifndef __LLAE_ARCHIVE_ZLIBUNCOMPRESS_H_INCLUDED__
#define __LLAE_ARCHIVE_ZLIBUNCOMPRESS_H_INCLUDED__

#include "zlibcommon.h"


namespace uv {
	class loop;
}

namespace archive {

	class zlibuncompress : public impl::zlibstream<zlibuncompress> {
		META_OBJECT
	private:
        std::atomic<bool> m_try_raw;
	public:
        static int process(z_stream* z,int flush,impl::zlibstream<zlibuncompress>&);
		zlibuncompress();
		~zlibuncompress();
		int init(uv::loop& l,int windowBits);
		bool is_error() const { return m_is_error; }
		
		bool init_inflate(lua::state& l,int argbase);
		

        static bool inflate(const void* src,size_t src_size,void* dst,size_t& dst_size);
		static void lbind(lua::state& l);
	};
	using zlibuncompress_ptr = common::intrusive_ptr<zlibuncompress>; 

    class zlibuncompress_inflate_read : public impl::zlibstream_read<zlibuncompress> {
        META_OBJECT
    public:
        zlibuncompress_inflate_read();
        static void lbind(lua::state& l);
        static lua::multiret new_inflate(lua::state& l);
    };
    using zlibuncompress_inflate_read_ptr = common::intrusive_ptr<zlibuncompress_inflate_read>;

    class zlibuncompress_gzip_read : public zlibuncompress_inflate_read {
        META_OBJECT
    protected:
        bool init_gzinflate(lua::state& l,int argbase);
    public:
        zlibuncompress_gzip_read();
        static lua::multiret new_gzip(lua::state& l);
    };
    using zlibuncompress_gzip_read_ptr = common::intrusive_ptr<zlibuncompress_gzip_read>;

    class zlibuncompress_to_stream : public impl::zlibstream_to_stream<zlibuncompress> {
        META_OBJECT
    private:
    public:
        explicit zlibuncompress_to_stream( uv::stream_ptr&& stream );
        static void lbind(lua::state& l);
        static lua::multiret new_inflate(lua::state& l);
    };
    using zlibuncompress_to_stream_ptr = common::intrusive_ptr<zlibuncompress_to_stream>;


}

#endif /*__LLAE_ARCHIVE_ZLIBUNCOMPRESS_H_INCLUDED__*/
