#include "uncompress.h"

#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::zstduncompress,meta::object)
META_OBJECT_INFO(archive::zstduncompress_read,archive::zstduncompress)
META_OBJECT_INFO(archive::zstduncompress_to_stream,archive::zstduncompress)

namespace archive {

	class zstd_decompress_work : public uv::lua_cont_work {
	protected:
	    llae::buffer_base_ptr m_src_data;
	    llae::buffer_ptr m_dst_data;
	    size_t m_result = 0;
	protected:
        virtual bool init_decoder() {
            m_dst_data->set_len(0);
            return true;
        }
	    virtual void on_work() override {

            if (!init_decoder()) {
                return;
            }
	    	
            m_result = ZSTD_decompress(m_dst_data->get_base(),m_dst_data->get_capacity(),
                m_src_data->get_base(),m_src_data->get_len());
            if (!ZSTD_isError(m_result)) {
                m_dst_data->set_len(m_result);
                m_result = 0;
            }
	    	
	    }
	    virtual int resume_args(lua::state& l,int status) override {
	        int args;
	        if (status < 0) {
	        	printf("status failed %d\n", status);
	            l.pushnil();
	            uv::push_error(l,status);
	            args = 2;
	        } else if (m_result != 0) {
	            l.pushnil();
	            impl::ZSTD::pusherror(l,ZSTD_getErrorCode(m_result));
	            args = 2;
	        } else {
	            lua::push(l,std::move(m_dst_data));
	            args = 1;
	        }
	        return args;
	    }
	public:
	    explicit zstd_decompress_work(lua::ref&& cont,llae::buffer_base_ptr&& src,size_t dst_buffer_size) : uv::lua_cont_work(std::move(cont)),m_src_data(std::move(src)) {
	        m_dst_data = llae::buffer::alloc(dst_buffer_size);
        }
        ~zstd_decompress_work() {
        }
	};

    zstduncompress::zstduncompress() {
		
    }

    ZSTD_ErrorCode zstduncompress::process(impl::ZSTD::stream* z,int flush,impl::compressionstream<zstduncompress,impl::ZSTD>& s) {
        auto res = ZSTD_decompressStream(z->decompress,&z->output,&z->input);
        if (res == 0)
            return impl::ZSTD::STREAM_END;
        return ZSTD_getErrorCode(res);
    }

    size_t zstduncompress::init(uv::loop& l) {
        auto ret = ZSTD_initDStream(m_z.decompress);
        if (ZSTD_isError(ret)) {
        	return ret;
        }
        init_common(l);
        return 0;
    }

    bool zstduncompress::init_decompress(lua::state& l,int argbase) {
        init(llae::app::get(l).loop());
        return true;
    }
    
    zstduncompress::~zstduncompress() {
    }

    
    lua::multiret zstduncompress::decompress(lua::state& l) {
    	auto buf = llae::buffer_base::get(l,1,true);
        auto dst_size = l.checkinteger(2);
	    if (!buf) {
	        l.argerror(1,"need data");
	    }
	    if (!l.isyieldable()) {
	        l.pushnil();
	        l.pushstring("decompress is async");
	        return {2};
	    }
	    
	    {
	        l.pushthread();
	        lua::ref cont;
	        cont.set(l);
	        common::intrusive_ptr<zstd_decompress_work> work(new zstd_decompress_work(std::move(cont),std::move(buf),dst_size));
	        int r = work->queue_work(l);
	        if (r < 0) {
	            work->reset(l);
	            l.pushnil();
	            uv::push_error(l,r);
	            return {2};
	        }
	    }
	    
	    l.yield(0);
	    return {0};
    }


    void zstduncompress::lbind(lua::state& l) {
        lua::bind::function(l,"write",&zstduncompress::write);
        lua::bind::function(l,"finish",&zstduncompress::finish);
        lua::bind::function(l,"send",&zstduncompress::send);
    }
    

    zstduncompress_read::zstduncompress_read() {

    }


    void zstduncompress_read::lbind(lua::state& l) {
        lua::bind::function(l,"read",&zstduncompress_read::read);
        lua::bind::function(l,"read_buffer",&zstduncompress_read::read_buffer);
    }


    lua::multiret zstduncompress_read::new_decompress(lua::state& l) {
        zstduncompress_read_ptr res(new zstduncompress_read());
        if (!res->init_decompress(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }



    zstduncompress_to_stream::zstduncompress_to_stream( uv::stream_ptr&& stream ) : compressionstream_to_stream<zstduncompress>(std::move(stream)) {

    }

    void zstduncompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &zstduncompress_to_stream::shutdown);
    }

    lua::multiret zstduncompress_to_stream::new_decompress(lua::state& l) {
        auto s = lua::stack<uv::stream_ptr>::get(l,1);
        zstduncompress_to_stream_ptr res(new zstduncompress_to_stream(std::move(s)));
        if (!res->init_decompress(l,2)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }

   
   
}
