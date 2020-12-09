#include "zlibuncompress.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::zlibuncompress,meta::object)
META_OBJECT_INFO(archive::zlibuncompress_read,archive::zlibuncompress)
META_OBJECT_INFO(archive::zlibuncompress_to_stream,archive::zlibuncompress)

namespace archive {


	zlibuncompress::zlibuncompress() {
        m_try_raw = false;
  	}

    int zlibuncompress::process(z_stream* z,int flush,impl::zlibstream<zlibuncompress>& s) {
        auto& self(static_cast<zlibuncompress&>(s));
        auto origin = z->next_in;
        int r = inflate(z, flush);
        if (r == Z_DATA_ERROR && z->total_out==0 && self.m_try_raw) {
            auto avail_in = z->avail_in + (z->next_in-origin);
            self.m_try_raw = false;
            inflateReset(z);
            r = inflateInit2(z, -MAX_WBITS);
            z->avail_in = avail_in;
            z->next_in = origin;
            if ( r!=Z_OK) return r;
            r = inflate(z, flush);
        }
        return r;
    }

	int zlibuncompress::init(uv::loop& l,int windowBits) {
		int r = inflateInit2(&m_z,windowBits);
		if (r!=Z_OK) {
			m_is_error = true;
			return r;
		}
        init_common(l);
		return r;
	}

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

	bool zlibuncompress::init_inflate(lua::state& l,int argbase) {
		int r = init(llae::app::get(l).loop(), MAX_WBITS);
		if (r != Z_OK) {
			l.pushnil();
			impl::pushzerror(l,r);
			return false;
		}
        if (l.gettop()>=argbase && l.isboolean(argbase)) {
            m_try_raw = l.toboolean(argbase);
        }
		return true;
	}
	bool zlibuncompress::init_gzinflate(lua::state& l,int argbase) {
		int r = init(llae::app::get(l).loop(),MAX_WBITS+16);
		if (r != Z_OK) {
			l.pushnil();
			impl::pushzerror(l,r);
			return false;
		}
//		gz_header header;
//        header.text = 0;
//        header.time = 0;
//		header.xflags = 0;
//		header.os = 3; // unix
//		header.extra = Z_NULL;
//		header.extra_len = 0;
//		header.extra_max = 0;
//        header.name = 0;
//		header.name_max = 0;
//		header.comment = Z_NULL;
//		header.comm_max = 0;
//		header.hcrc = 1;
//		header.done = 0;
//
//		r = deflateSetHeader(&m_z,&header);
//		if (r != Z_OK) {
//			l.pushnil();
//			pushzerror(l,r);
//			return false;
//		}
		return true;
	}
    zlibuncompress::~zlibuncompress() {
		inflateEnd(&m_z);
	}

	


	void zlibuncompress::lbind(lua::state& l) {
		lua::bind::function(l,"write",&zlibuncompress::write);
		lua::bind::function(l,"finish",&zlibuncompress::finish);
		lua::bind::function(l,"send",&zlibuncompress::send);
	}
	

    zlibuncompress_read::zlibuncompress_read() {

    }


    void zlibuncompress_read::lbind(lua::state& l) {
        lua::bind::function(l,"read",&zlibuncompress_read::read);
        lua::bind::function(l,"read_buffer",&zlibuncompress_read::read_buffer);
    }


    lua::multiret zlibuncompress_read::new_inflate(lua::state& l) {
        zlibuncompress_read_ptr res(new zlibuncompress_read());
        if (!res->init_inflate(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }

    lua::multiret zlibuncompress_read::new_gzip(lua::state& l) {
        zlibuncompress_read_ptr res(new zlibuncompress_read());
        if (!res->init_gzinflate(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }


    zlibuncompress_to_stream::zlibuncompress_to_stream( uv::stream_ptr&& stream ) : zlibstream_to_stream<zlibuncompress>(std::move(stream)) {

    }

    void zlibuncompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &zlibuncompress_to_stream::shutdown);
    }

    lua::multiret zlibuncompress_to_stream::new_inflate(lua::state& l) {
        auto s = lua::stack<uv::stream_ptr>::get(l,1);
        zlibuncompress_to_stream_ptr res(new zlibuncompress_to_stream(std::move(s)));
        if (!res->init_inflate(l,2)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }
   
}
