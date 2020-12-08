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
	
	public:
        static int process(z_stream* z,int flush) {
            return inflate(z,flush);
        }
		zlibuncompress();
		~zlibuncompress();
		int init(uv::loop& l,int windowBits);
		bool is_error() const { return m_is_error; }
		
		bool init_inflate(lua::state& l,int argbase);
		bool init_gzinflate(lua::state& l,int argbase);
		static void lbind(lua::state& l);
	};
	using zlibuncompress_ptr = common::intrusive_ptr<zlibuncompress>; 

    class zlibuncompress_read : public impl::zlibstream_read<zlibuncompress> {
        META_OBJECT
    public:
        zlibuncompress_read();
        static void lbind(lua::state& l);
        static lua::multiret new_inflate(lua::state& l);
        static lua::multiret new_gzip(lua::state& l);
    };
    using zlibuncompress_read_ptr = common::intrusive_ptr<zlibuncompress_read>;

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
