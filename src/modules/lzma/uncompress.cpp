#include "uncompress.h"

#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::lzmauncompress,meta::object)
META_OBJECT_INFO(archive::lzmauncompress_read,archive::lzmauncompress)
META_OBJECT_INFO(archive::lzmauncompress_to_stream,archive::lzmauncompress)

namespace archive {

	class decompress_work : public uv::lua_cont_work {
	private:
	    uv::buffer_base_ptr m_src_data;
	    uv::buffer_ptr m_dst_data;
	    int m_result = 0;
	    lzma_stream m_z;
	protected:
	    virtual void on_work() override {

	    	
	    	m_result = lzma_auto_decoder(&m_z,UINT64_MAX,0);
	    	m_dst_data->set_len(0);
	    	if (m_result !=0) {
	    		return;
	    	}
	    	m_z.next_in = static_cast<const uint8_t*>(m_src_data->get_base());
	    	m_z.avail_in = m_src_data->get_len();
	    	m_z.next_out = static_cast<uint8_t*>(m_dst_data->get_base());
	    	m_z.avail_out = m_dst_data->get_capacity();
	    	size_t write_size = m_dst_data->get_capacity();
            auto act = LZMA_RUN;
	    	while(true) {
                if (m_z.avail_out == 0) {
                    m_dst_data = m_dst_data->realloc(m_dst_data->get_capacity() * 2);
                    write_size = m_dst_data->get_capacity() - m_dst_data->get_len();
                    m_z.next_out = static_cast<uint8_t*>(m_dst_data->get_base())+m_dst_data->get_len();
                    m_z.avail_out = write_size;
                }
	    		m_result = ::lzma_code(&m_z,LZMA_RUN);
	    		m_dst_data->set_len(m_z.total_out);
	    		if (m_result == LZMA_OK  || m_result == LZMA_STREAM_END) {
                    if (act == LZMA_FINISH) {
                        m_result = LZMA_OK;
                        return;
                    }
	    		} else {
                    // error
                    break;
                }
                if (m_z.avail_in == 0) {
                    act = LZMA_FINISH;
                }
	    	}
	    }
	    virtual int resume_args(lua::state& l,int status) override {
	        int args;
	        if (status < 0) {
	            l.pushnil();
	            uv::push_error(l,status);
	            args = 2;
	        } else if (m_result != LZMA_OK) {
	            l.pushnil();
	            impl::LZMA::pusherror(l,m_z,m_result);
	            l.pushstring("decompress failed");
	            args = 2;
	        } else {
	            lua::push(l,std::move(m_dst_data));
	            args = 1;
	        }
	        return args;
	    }
	public:
	    explicit decompress_work(lua::ref&& cont,uv::buffer_base_ptr&& src,size_t dst_buffer_size) : uv::lua_cont_work(std::move(cont)),m_src_data(std::move(src)) {
	        m_dst_data = uv::buffer::alloc(dst_buffer_size);
	        m_z = LZMA_STREAM_INIT;
	    }
	};


    lzmauncompress::lzmauncompress() {
		lzma_stream tmp = LZMA_STREAM_INIT;
		m_z = tmp;
    }

    int lzmauncompress::process(impl::LZMA::stream* z,int flush,impl::compressionstream<lzmauncompress,impl::LZMA>& s) {
        int r = ::lzma_code(z,static_cast<lzma_action>(flush));
        return r;
    }

    int lzmauncompress::init(uv::loop& l) {
        auto ret = lzma_auto_decoder(&m_z,UINT64_MAX,0);
        if (ret != LZMA_OK) {
        	return ret;
        }
        init_common(l);
        return 0;
    }

    bool lzmauncompress::init_decompress(lua::state& l,int argbase) {
        init(llae::app::get(l).loop());
        return true;
    }
    
    lzmauncompress::~lzmauncompress() {
    	lzma_end(&m_z);
    }

    
    lua::multiret lzmauncompress::decompress(lua::state& l) {
    	auto buf = uv::buffer_base::get(l,1,true);
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
	        common::intrusive_ptr<decompress_work> work(new decompress_work(std::move(cont),std::move(buf),dst_size));
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


    void lzmauncompress::lbind(lua::state& l) {
        lua::bind::function(l,"write",&lzmauncompress::write);
        lua::bind::function(l,"finish",&lzmauncompress::finish);
        lua::bind::function(l,"send",&lzmauncompress::send);
    }
    

    lzmauncompress_read::lzmauncompress_read() {

    }


    void lzmauncompress_read::lbind(lua::state& l) {
        lua::bind::function(l,"read",&lzmauncompress_read::read);
        lua::bind::function(l,"read_buffer",&lzmauncompress_read::read_buffer);
    }


    lua::multiret lzmauncompress_read::new_decompress(lua::state& l) {
        lzmauncompress_read_ptr res(new lzmauncompress_read());
        if (!res->init_decompress(l,1)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }



    lzmauncompress_to_stream::lzmauncompress_to_stream( uv::stream_ptr&& stream ) : compressionstream_to_stream<lzmauncompress>(std::move(stream)) {

    }

    void lzmauncompress_to_stream::lbind(lua::state &l) {
        lua::bind::function(l, "shutdown", &lzmauncompress_to_stream::shutdown);
    }

    lua::multiret lzmauncompress_to_stream::new_decompress(lua::state& l) {
        auto s = lua::stack<uv::stream_ptr>::get(l,1);
        lzmauncompress_to_stream_ptr res(new lzmauncompress_to_stream(std::move(s)));
        if (!res->init_decompress(l,2)) {
            return {2};
        }
        lua::push(l,std::move(res));
        return {1};
    }

   
   
}
