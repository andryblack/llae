#ifndef __LLAE_ARCHIVE_ZLIBCOMPRESS_H_INCLUDED__
#define __LLAE_ARCHIVE_ZLIBCOMPRESS_H_INCLUDED__

#include "zlibcommon.h"

namespace uv {
	class loop;
}

namespace archive {

	class zlibcompress : public impl::zlibstream<zlibcompress> {
		META_OBJECT
	private:
		
	public:
        static int process(z_stream* z,int flush) {
            return deflate(z,flush);
        }
		zlibcompress();
		~zlibcompress();
		int init(uv::loop& l,int level,int method,int windowBits,int memLevel,int strategy);
		
		bool init_deflate(lua::state& l,int argbase);
		bool init_gzdeflate(lua::state& l,int argbase);
		static void lbind(lua::state& l);
	};
	using zlibcompress_ptr = common::intrusive_ptr<zlibcompress>; 

	class zlibcompress_read : public impl::zlibstream_read<zlibcompress> {
		META_OBJECT
	public:
		zlibcompress_read();
		static void lbind(lua::state& l);
		static lua::multiret new_deflate(lua::state& l);
		static lua::multiret new_gzip(lua::state& l);
	};
	using zlibcompress_read_ptr = common::intrusive_ptr<zlibcompress_read>; 

	class zlibcompress_to_stream : public impl::zlibstream_to_stream<zlibcompress> {
		META_OBJECT
	private:
	public:
		explicit zlibcompress_to_stream( uv::stream_ptr&& stream );
    	static void lbind(lua::state& l);
		static lua::multiret new_deflate(lua::state& l);
	};
    using zlibcompress_to_stream_ptr = common::intrusive_ptr<zlibcompress_to_stream>;

	
}

#endif /*__LLAE_ARCHIVE_ZLIBCOMPRESS_H_INCLUDED__*/
