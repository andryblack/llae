#include "gzstream.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "uv/loop.h"
#include "uv/async.h"
#include "uv/fs.h"
#include "llae/app.h"
#include "lua/bind.h"

META_OBJECT_INFO(archive::zlibcompress,meta::object)
META_OBJECT_INFO(archive::zlibcompress_read,archive::zlibcompress)
META_OBJECT_INFO(archive::zlibcompress_to_stream,archive::zlibcompress)

namespace archive {

	static const size_t COMPRESSED_BLOCK_SIZE = 1024 * 4;

	class zlibcompress::compress_work : public uv::work {
	protected:
		zlibcompress_ptr m_compress;
		int m_z_status = Z_OK;
		uv::buffer_ptr m_out;
		void flush_out(z_stream& z) {
			if (m_out) {
				m_compress->add_compressed(std::move(m_out));
			}
			m_out = std::move(uv::buffer::alloc(COMPRESSED_BLOCK_SIZE));
			z.avail_out = m_out->get_len();
			z.next_out = static_cast<Bytef*>(m_out->get_base());
		}
		virtual void on_after_work(int status) override {
			auto& s(llae::app::get(get_loop()).lua());
			if (m_compress) {
                m_compress->on_compress_complete(s,status,m_z_status);
			}
		}
		explicit compress_work(zlibcompress_ptr&& compress) : m_compress(std::move(compress)) {}
	};

	class zlibcompress::compress_buffers : public zlibcompress::compress_work {
	private:
		uv::write_buffers m_buffers;
	protected:
		
		virtual void on_work() override {
			auto& z(m_compress->m_z);
			for (auto& b:m_buffers.get_buffers()) {
				z.next_in = reinterpret_cast<Bytef*>(b.base);
				z.avail_in = b.len;
				while(z.avail_in) {
					if (!z.avail_out) {
						flush_out(z);
					}
					int r = deflate(&z,Z_NO_FLUSH);
					if (r != Z_OK && r != Z_BUF_ERROR) {
						m_z_status = r;
						return;
					}
				}
			}
            if (m_out) {
                m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
                m_compress->add_compressed(std::move(m_out));
            }
            z.avail_out = 0;
		}
		virtual void on_after_work(int status) override {
			auto& s(llae::app::get(get_loop()).lua());
			m_buffers.reset(s);
			compress_work::on_after_work(status);
		}
	public:
		compress_buffers(zlibcompress_ptr&& stream) : zlibcompress::compress_work(std::move(stream)) {
		}
		void put(lua::state& l) {
			m_buffers.put(l);
		}
	};
	
	class zlibcompress::compress_finish : public zlibcompress::compress_buffers {
	protected:
		virtual void on_work() override {
			compress_buffers::on_work();
			if (m_z_status != Z_OK)
				return;
			auto& z(m_compress->m_z);
			while (true) {
				if (!z.avail_out) {
					flush_out(z);
				}
				int r = deflate(&z,Z_FINISH);
				if (r == Z_STREAM_END) {
                    if (m_out) {
                        m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
                        m_compress->add_compressed(std::move(m_out));
                    }
					return;
				}
				if (r != Z_OK && r != Z_BUF_ERROR) {
					m_z_status = r;
					return;
				}
			}
		}
		virtual void on_after_work(int status) override{
			compress_buffers::on_after_work(status);
			if (m_compress) {
				m_compress->on_finish_compress(llae::app::get(get_loop()).lua());
			}
		}
	public:
		compress_finish(zlibcompress_ptr&& stream) : compress_buffers(std::move(stream)) {}
	};

	class zlibcompress::write_file_pipe : public zlibcompress::compress_work {
	private:
		static const size_t BUFFER_SIZE = 1024 * 8;
		uv::file_ptr m_file;
		
		int64_t m_offset = 0;

		uv_fs_t	m_fs_req;
		uv::buffer_ptr m_read_buffer;
		uv::buffer_ptr m_compress_buffer;

		bool m_compress_active = false;
		bool m_read_active = false;
		bool m_eof = false;
		
		static void fs_cb(uv_fs_t* req) {
			write_file_pipe* self = static_cast<write_file_pipe*>(req->data);
			self->on_read(llae::app::get(req->loop).loop(),uv_fs_get_result(req));
		}
		void on_end(uv::loop& l,int status) {
			if (m_compress_active || m_read_active)
				return;
			m_compress->on_finish_compress(llae::app::get(l).lua());
			remove_ref();
		}
		
		int start_compress(uv::loop& l) {
			m_compress_active = true;
            if (!m_eof) {
                m_compress_buffer = std::move(m_read_buffer);
            } else {
                m_compress_buffer.reset();
            }
			int r = queue_work(l);
			if (r < 0) {
				m_compress_active = false;
			}
			return r;
		}

		virtual void on_work() override {
			auto& z(m_compress->m_z);
			if (m_compress_buffer) {
				z.next_in = reinterpret_cast<Bytef*>(m_compress_buffer->get_base());
				z.avail_in = m_compress_buffer->get_len();
				while(z.avail_in) {
					if (!z.avail_out) {
						flush_out(z);
					}
					int r = deflate(&z,Z_NO_FLUSH);
					if (r != Z_OK && r != Z_BUF_ERROR) {
						m_z_status = r;
						return;
					}
				}
			} else {
				while (true) {
					if (!z.avail_out) {
						flush_out(z);
					}
					int r = deflate(&z,Z_FINISH);
					if (r == Z_STREAM_END) {
	                    if (m_out) {
	                        m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
	                        m_compress->add_compressed(std::move(m_out));
	                    }
						return;
					}
					if (r != Z_OK && r != Z_BUF_ERROR) {
						m_z_status = r;
						return;
					}
				}
			}
		}
		virtual void on_after_work(int status) override {
			m_compress_active = false;
			m_read_buffer = std::move(m_compress_buffer);
            if (m_read_buffer) {
                m_read_buffer->set_len(BUFFER_SIZE);
            }
			auto& l(llae::app::get(get_loop()).loop());
            auto& s(llae::app::get(get_loop()).lua());
            if (status < 0 || ((m_z_status!=Z_OK&&m_z_status!=Z_BUF_ERROR))) {
                m_compress->on_compress_complete(s,status,m_z_status);
            } else if (m_eof){
                m_compress->on_compress_complete(s,status,m_z_status);
                on_end(l,status);
            } else if (status>=0 && (m_z_status==Z_OK||m_z_status==Z_BUF_ERROR)) {
                int r = start_read(l);
                if (r < 0) {
                    on_end(l,r);
                }
            }
		}
		int start_read(uv::loop& l) {
			m_read_active = true;
			if (!m_read_buffer) {
				m_read_buffer = uv::buffer::alloc(BUFFER_SIZE);
			}
			int r = uv_fs_read(l.native(),&m_fs_req,m_file->get(),
				m_read_buffer->get(),1,m_offset,&fs_cb);
			if (r < 0) {
				m_read_active = false;
			}
			return r;
		}

		void on_read(uv::loop& l,int status) {
			m_read_active = false;
			if (status < 0) {
				return on_end(l,status);
			}
			if (status > 0) {
				m_offset += status;
				m_read_buffer->set_len(status);
				int r = start_compress(l);
				if (r < 0) 
					return on_end(l,r);
			} else {
				m_eof = true;
				int r = start_compress(l);
				if (r < 0) 
					return on_end(l,r);
			}
		}
		
	protected:
		~write_file_pipe() {
		}
	public:
		write_file_pipe(zlibcompress_ptr&& stream,uv::file_ptr&& f) : 
			zlibcompress::compress_work(std::move(stream)),m_file(std::move(f)) {
			m_fs_req.data = this;
		}
		int start(uv::loop& l) {
			add_ref();
			int r = start_read(l);
			if (r < 0) {
				remove_ref();
			}
			return r;
		}
	};

	class zlibcompress::async_resume_read : public uv::async {
		zlibcompress* m_self;
	public:
		async_resume_read(uv::loop& l,zlibcompress* s);
		virtual void on_async() override;
		void reset();
	};

	zlibcompress::async_resume_read::async_resume_read(uv::loop& l,zlibcompress* s) : uv::async(l),m_self(s) {

	}
	void zlibcompress::async_resume_read::on_async() {
		if (m_self) {
			m_self->continue_read(llae::app::get(get_loop()).lua());
		}
	}
	void zlibcompress::async_resume_read::reset() {
		m_self = nullptr;
	}

	static void pushzerror(lua::state& l,int z_err) {
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

	zlibcompress::zlibcompress() {
        memset(&m_z, 0, sizeof(m_z));
	}

	int zlibcompress::init(uv::loop& l,int level,int method,int windowBits,int memLevel,int strategy) {
		int r = deflateInit2(&m_z,level,method,windowBits,memLevel,strategy);
		if (r!=Z_OK) {
			m_is_error = true;
			return r;
		}
		m_read_resume.reset( new async_resume_read(l,this) );
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
			pushzerror(l,r);
			return false;
		}
		return true;
	}
	zlibcompress::~zlibcompress() {
		if (m_read_resume) {
			m_read_resume->reset();
		}
		deflateEnd(&m_z);
	}

	void zlibcompress::on_finish_compress(lua::state& l) {
		m_finished = true;
		continue_read(l);
	}



	void zlibcompress::on_compress_complete(lua::state& l,int status,int z_err) {
		if (status < 0 || (z_err != Z_OK && z_err != Z_BUF_ERROR)) {
			m_is_error = true;
		}
		if (m_write_cont.valid()) {
			m_write_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				args = 2;
			} else if (z_err != Z_OK && z_err != Z_BUF_ERROR) {
				toth.pushnil();
				if (m_z.msg) {
					toth.pushstring(m_z.msg);
				} else {
					pushzerror(toth,z_err);
				}
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			m_write_cont.reset(l);
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
		}
		continue_read(l);
	}

	void zlibcompress::add_compressed(uv::buffer_ptr&& b) {
		uv::scoped_lock l(m_mutex);
		m_compressed.emplace_back(std::move(b));
		if (m_read_resume) {
			m_read_resume->send();
		}
	}

	lua::multiret zlibcompress::write(lua::state& l) {
		if (is_error()) {
            l.pushnil();
            l.pushstring("compress_steam::write is error");
            return {2};
        }
        if (m_finished) {
            l.pushnil();
            l.pushstring("compress_steam::write is finished");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("compress_steam::write is async");
            return {2};
        }
        if (m_write_cont.valid()) {
            l.pushnil();
            l.pushstring("compress_steam::write async not completed");
            return {2};
        }
        {
            l.pushthread();
            m_write_cont.set(l);
            
            common::intrusive_ptr<compress_buffers> work(new compress_buffers(zlibcompress_ptr(this)));
            l.pushvalue(2);
            work->put(l);
            int r = work->queue_work(llae::app::get(l).loop());
            if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
        }
        l.yield(0);
        return {0};
	}

	lua::multiret zlibcompress::finish(lua::state& l) {
		if (is_error()) {
            l.pushnil();
            l.pushstring("zlibcompress::finish is error");
            return {2};
        }
        if (m_finished) {
            l.pushnil();
            l.pushstring("zlibcompress::finish is finished");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("zlibcompress::finish is async");
            return {2};
        }
        if (m_write_cont.valid()) {
            l.pushnil();
            l.pushstring("zlibcompress::finish async not completed");
            return {2};
        }
        {
            l.pushthread();
            m_write_cont.set(l);
            
            common::intrusive_ptr<compress_buffers> work(new compress_finish(zlibcompress_ptr(this)));
            if (l.gettop()>1) {
            	l.pushvalue(2);
            	work->put(l);
            }
            int r = work->queue_work(llae::app::get(l).loop());
            if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
        }
        l.yield(0);
        return {0};
	}

	lua::multiret zlibcompress::send(lua::state& l) {
		if (is_error()) {
            l.pushnil();
            l.pushstring("zlibcompress::send is error");
            return {2};
        }
        if (m_finished) {
            l.pushnil();
            l.pushstring("zlibcompress::send is finished");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("zlibcompress::send is async");
            return {2};
        }
        if (m_write_cont.valid()) {
            l.pushnil();
            l.pushstring("zlibcompress::send async not completed");
            return {2};
        }
		{
			auto f = lua::stack<common::intrusive_ptr<uv::file> >::get(l,2);
			if (!f) {
				l.argerror(2,"uv::file expected");
			}
			l.pushthread();
			m_write_cont.set(l);

			common::intrusive_ptr<write_file_pipe> req{new write_file_pipe(zlibcompress_ptr(this),
				std::move(f))};
			
			int r = req->start(llae::app::get(l).loop());
			if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}


	void zlibcompress::lbind(lua::state& l) {
		lua::bind::function(l,"write",&zlibcompress::write);
		lua::bind::function(l,"finish",&zlibcompress::finish);
		lua::bind::function(l,"send",&zlibcompress::send);
	}
	

	zlibcompress_read::zlibcompress_read() {

	}

	lua::multiret zlibcompress_read::read(lua::state& l) {
		if (is_error()) {
            l.pushnil();
            l.pushstring("compress_steam::read is error");
            return {2};
        }
        if (m_read_cont.valid()) {
            l.pushnil();
            l.pushstring("compress_steam::read async not completed");
            return {2};
        }
        uv::buffer_ptr buf;
        {
        	uv::scoped_lock l(m_mutex);
        	if (!m_compressed.empty()) {
        		buf = m_compressed.front();
        		m_compressed.erase(m_compressed.begin());
        	}
        }
        if (buf) {
        	l.pushlstring(static_cast<const char*>(buf->get_base()),buf->get_len());
        	return {1};
        }
        if (m_finished) {
        	l.pushnil();
        	return {1};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("compress_steam::read is async");
            return {2};
        }
        
        {
            l.pushthread();
            m_read_buffer = false;
            m_read_cont.set(l);
        }
        
        l.yield(0);
        return {0};
	}

	lua::multiret zlibcompress_read::read_buffer(lua::state& l) {
		if (is_error()) {
            l.pushnil();
            l.pushstring("compress_steam::read is error");
            return {2};
        }
        if (m_read_cont.valid()) {
            l.pushnil();
            l.pushstring("compress_steam::read async not completed");
            return {2};
        }
        uv::buffer_ptr buf;
        {
        	uv::scoped_lock l(m_mutex);
        	if (!m_compressed.empty()) {
        		buf = m_compressed.front();
        		m_compressed.erase(m_compressed.begin());
        	}
        }
        if (buf) {
        	lua::push(l,std::move(buf));
        	return {1};
        }
        if (m_finished) {
        	l.pushnil();
        	return {1};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("compress_steam::read is async");
            return {2};
        }
        
        {
            l.pushthread();
            m_read_buffer = true;
            m_read_cont.set(l);
        }
        
        l.yield(0);
        return {0};
	}

	void zlibcompress_read::continue_read(lua::state& l) {
        if (!m_read_cont.valid()) {
            return;
        }
		uv::buffer_ptr buf;
        {
        	uv::scoped_lock l(m_mutex);
        	if (!m_compressed.empty()) {
        		buf = std::move(m_compressed.front());
        		m_compressed.erase(m_compressed.begin());
        	}
        }
        if (!buf && !m_finished && !is_error()) {
        	return;
        }
        m_read_cont.push(l);
        auto toth = l.tothread(-1);
        l.pop(1);// thread
        int args;
        if (is_error()) {
            m_is_error = true;
            toth.pushnil();
            toth.pushstring("compression error");
            args = 2;
        } else {
            if (buf) {
            	if (m_read_buffer) {
            		lua::push(toth,std::move(buf));
            	} else {
            		toth.pushlstring(static_cast<const char*>(buf->get_base()),buf->get_len());
            	}
            } else {
                toth.pushnil();
            }
            args = 1;
        }
        m_read_cont.reset(l);
        auto s = toth.resume(l,args);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }
	}

	void zlibcompress_read::lbind(lua::state& l) {
		lua::bind::function(l,"read",&zlibcompress_read::read);
		lua::bind::function(l,"read_buffer",&zlibcompress_read::read_buffer);
	}


	lua::multiret zlibcompress_read::new_deflate(lua::state& l) {
		zlibcompress_read_ptr res(new zlibcompress_read());
		if (!res->init_deflate(l,1)) {
			return {2};
		}
		lua::push(l,std::move(res));
		return {1};
	}


	zlibcompress_to_stream::write_buffer_req::write_buffer_req(zlibcompress_to_stream_ptr&& self,uv::buffer_ptr&& b) : 
		uv::write_buffer_req(uv::stream_ptr(self->m_stream),std::move(b)),
		m_self(std::move(self)) {

		}

	void zlibcompress_to_stream::write_buffer_req::on_write(int status) {
		if (status < 0) {
			uv::print_error(status);
		} else {
			m_self->continue_read(llae::app::get(get_loop()).lua());
		}
		m_self.reset();
	}

	zlibcompress_to_stream::shutdown_req::shutdown_req(zlibcompress_to_stream_ptr&& self) : 
		uv::shutdown_req(uv::stream_ptr(self->m_stream)),
		m_self(std::move(self)) {

		}

	void zlibcompress_to_stream::shutdown_req::on_shutdown(int status) {
		if (status < 0) {
			uv::print_error(status);
		} else {
			m_self->continue_read(llae::app::get(get_loop()).lua());
		}
		m_self.reset();
	}

	zlibcompress_to_stream::zlibcompress_to_stream( uv::stream_ptr&& stream ) : m_stream(std::move(stream)) {

	}

	void zlibcompress_to_stream::continue_read(lua::state& l) {
		if (!m_stream) {
			return;
		}
		uv::buffer_ptr buf;
		while (true) {
			{
	        	uv::scoped_lock l(m_mutex);
	        	if (!m_compressed.empty()) {
	        		buf = std::move(m_compressed.front());
	        		m_compressed.erase(m_compressed.begin());
	        	}
	        }
	        if (!buf && !m_finished && !is_error()) {
	        	return;
	        }
	        if (is_error()) {
				return;
			}
			if (buf) {
				common::intrusive_ptr<write_buffer_req> req(new write_buffer_req(
					zlibcompress_to_stream_ptr(this),
					std::move(buf)));
                buf.reset();
				int st = req->write();
				if (st < 0) {
					uv::print_error(st);
				}
			} else if (m_finished && m_need_shutdown) {
				common::intrusive_ptr<shutdown_req> req(new shutdown_req(zlibcompress_to_stream_ptr(this)));
				int st = req->shutdown();
				if (st < 0) {
					uv::print_error(st);
				} else {
					m_stream.reset();
				}
            } else {
                break;
            }
		}
		
	}

    lua::multiret zlibcompress_to_stream::shutdown(lua::state &l) {
        m_need_shutdown = true;
        return finish(l);
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
