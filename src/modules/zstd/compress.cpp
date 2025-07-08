#include "compress.h"
#include "uncompress.h"
#include "lua/bind.h"

namespace archive {

	class zstd_compress_work : public uv::lua_cont_work {
	protected:
	    llae::buffer_base_ptr m_src_data;
	    llae::buffer_ptr m_dst_data;
	    int m_compression_level = 0;
	    size_t m_result = 0;
	protected:
	    virtual void on_work() override {

	    	auto size = ZSTD_compressBound(m_src_data->get_len());
	    	m_dst_data = llae::buffer::alloc(size);
            m_result = ZSTD_compress(m_dst_data->get_base(),m_dst_data->get_capacity(),
                m_src_data->get_base(),m_src_data->get_len(),m_compression_level);
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
	    explicit zstd_compress_work(lua::ref&& cont,llae::buffer_base_ptr&& src,int level) : uv::lua_cont_work(std::move(cont)),m_src_data(std::move(src)),m_compression_level(level) {
	    }
        ~zstd_compress_work() {
        }
	};

	lua::multiret zstd_compress(lua::state& l) {
		auto buf = llae::buffer_base::get(l,1,true);
        if (!buf) {
	        l.argerror(1,"need data");
	    }
	    auto level = l.optinteger(2,ZSTD_CLEVEL_DEFAULT);
	    if (!l.isyieldable()) {
	        l.pushnil();
	        l.pushstring("compress is async");
	        return {2};
	    }
	    
	    {
	        l.pushthread();
	        lua::ref cont;
	        cont.set(l);
	        common::intrusive_ptr<zstd_compress_work> work(new zstd_compress_work(std::move(cont),std::move(buf),level));
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
}
