#include "zlibcompress.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::zlibcompress,meta::object)
META_OBJECT_INFO(archive::zlibcompress_deflate_read,archive::zlibcompress)
META_OBJECT_INFO(archive::zlibcompress_gzip_read,archive::zlibcompress_deflate_read)
META_OBJECT_INFO(archive::zlibcompress_to_stream,archive::zlibcompress)

namespace archive {



	namespace impl {
		void pushzerror(lua::state& l,int z_err) {
			if (z_err == Z_ERRNO) {
				l.pushstring("Z_ERRNO");
			} else if(z_err == Z_STREAM_ERROR) {
				l.pushstring("Z_STREAM_ERROR");
			} else if(z_err == Z_DATA_ERROR) {
				l.pushstring("Z_DATA_ERROR");
			} else if(z_err == Z_MEM_ERROR) {
				l.pushstring("Z_MEM_ERROR");
			} else if(z_err == Z_BUF_ERROR) {
				l.pushstring("Z_BUF_ERROR");
			} else if(z_err == Z_VERSION_ERROR) {
				l.pushstring("Z_VERSION_ERROR");
			} else {
				l.pushstring("unknown");
			}
		}
	}

	zlibcompress::zlibcompress() {
        
	}

	int zlibcompress::init(uv::loop& l,int level,int method,int windowBits,int memLevel,int strategy) {
		int r = deflateInit2(&m_z,level,method,windowBits,memLevel,strategy);
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

	bool zlibcompress::init_deflate(lua::state& l,int argbase) {
		int level = l.optinteger(argbase+0,Z_DEFAULT_COMPRESSION);
		int r = init(llae::app::get(l).loop(),level,Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
                         Z_DEFAULT_STRATEGY);
		if (r != Z_OK) {
			l.pushnil();
			impl::pushzerror(l,r);
			return false;
		}
		return true;
	}
	template <class T>
	static T optinteger(lua::state& l,int arg,const char* name,T default_value) {
		auto t = l.getfield(arg,name);
		T res = default_value;
		if (t == lua::value_type::number) {
			res = l.tointeger(-1);
		}
		l.pop(1);
		return res;
	}
	static int optboolean(lua::state& l,int arg,const char* name,bool default_value) {
		auto t = l.getfield(arg,name);
		bool res = default_value;
		if (t == lua::value_type::boolean) {
			res = l.toboolean(-1);
		}
		l.pop(1);
		return res;
	}
	static const char* optstring(lua::state& l,int arg,const char* name,const char* default_value) {
		auto t = l.getfield(arg,name);
		const char* res = default_value;
		if (t == lua::value_type::string) {
			res = l.tostring(-1);
		}
		l.pop(1);
		return res;
	}
	bool zlibcompress_gzip_read::init_gzdeflate(lua::state& l,int argbase) {
		l.checktype(argbase+0,lua::value_type::table);

		int level = optinteger(l,argbase+0,"level",Z_DEFAULT_COMPRESSION);
		int r = init(llae::app::get(l).loop(),level,Z_DEFLATED, MAX_WBITS+16, DEF_MEM_LEVEL,
                         Z_DEFAULT_STRATEGY);
		if (r != Z_OK) {
			l.pushnil();
			impl::pushzerror(l,r);
			return false;
		}
		gz_header& header(m_header);

		header.text = optboolean(l,argbase+0,"text",false) ? 1:0;
		uv_timeval64_t tv;
		uv_gettimeofday(&tv);
		header.time = optinteger(l,argbase+0,"time",tv.tv_sec);
		header.xflags = 0;
		header.os = 3; // unix
		header.extra = Z_NULL;
		header.extra_len = 0;
		header.extra_max = 0;
        auto name = optstring(l,argbase+0,"filename",nullptr);
        Bytef namebuf[128];
        if (name) {
            memcpy(namebuf, name, std::min(strlen(name),sizeof(namebuf)-1));
            namebuf[sizeof(namebuf)-1]=0;
            header.name = namebuf;
        } else {
            header.name = Z_NULL;
        }
		header.name_max = 0;
		header.comment = Z_NULL;
		header.comm_max = 0;
		header.hcrc = 1;
		header.done = 0;

		r = deflateSetHeader(&m_z,&header);
		if (r != Z_OK) {
			l.pushnil();
			impl::pushzerror(l,r);
			return false;
		}
		return true;
	}
	zlibcompress::~zlibcompress() {
		deflateEnd(&m_z);
	}


	void zlibcompress::lbind(lua::state& l) {
		lua::bind::function(l,"write",&zlibcompress::write);
		lua::bind::function(l,"finish",&zlibcompress::finish);
		lua::bind::function(l,"send",&zlibcompress::send);
	}
	

    zlibcompress_deflate_read::zlibcompress_deflate_read() {

	}


	void zlibcompress_deflate_read::lbind(lua::state& l) {
		lua::bind::function(l,"read",&zlibcompress_deflate_read::read);
		lua::bind::function(l,"read_buffer",&zlibcompress_deflate_read::read_buffer);
	}


	lua::multiret zlibcompress_deflate_read::new_deflate(lua::state& l) {
        zlibcompress_gzip_read_ptr res(new zlibcompress_gzip_read());
		if (!res->init_deflate(l,1)) {
			return {2};
		}
		lua::push(l,std::move(res));
		return {1};
	}

    zlibcompress_gzip_read::zlibcompress_gzip_read() {

    }

	lua::multiret zlibcompress_gzip_read::new_gzip(lua::state& l) {
        zlibcompress_gzip_read_ptr res(new zlibcompress_gzip_read());
		if (!res->init_gzdeflate(l,1)) {
			return {2};
		}
		lua::push(l,std::move(res));
		return {1};
	}


	zlibcompress_to_stream::zlibcompress_to_stream( uv::stream_ptr&& stream ) : zlibstream_to_stream<zlibcompress>(std::move(stream)) {

	}

    void zlibcompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &zlibcompress_to_stream::shutdown);
    }
	
	lua::multiret zlibcompress_to_stream::new_deflate(lua::state& l) {
		auto s = lua::stack<uv::stream_ptr>::get(l,1);
		zlibcompress_to_stream_ptr res(new zlibcompress_to_stream(std::move(s)));
		if (!res->init_deflate(l,2)) {
			return {2};
		}
		lua::push(l,std::move(res));
		return {1};
	}
	
}
