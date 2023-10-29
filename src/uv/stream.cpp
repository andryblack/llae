#include "stream.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "llae/app.h"
#include "luv.h"
#include "fs.h"
#include "write_file_pipe.h"
#include <iostream>

META_OBJECT_INFO(uv::stream,uv::handle)

namespace uv {

    

	write_req::write_req(stream_ptr&& s) : m_stream(std::move(s)) {
		attach(reinterpret_cast<uv_req_t*>(&m_write));
	}
	write_req::~write_req() {
	}

	void write_req::write_cb(uv_write_t* req, int status) {
		write_req* self = static_cast<write_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_write(status);
		self->remove_ref();
	}

	int write_req::start_write(const uv_buf_t* buffers,size_t count) {
		add_ref();
		int r = uv_write(&m_write,m_stream->get_stream(),
                         buffers,count,&write_req::write_cb);
		if (r < 0) {
			remove_ref();
		}
		return r;
	}


	write_lua_req::write_lua_req(stream_ptr&& s,lua::ref&& cont) : write_req(std::move(s)),m_cont(std::move(cont)) {
	}

	void write_lua_req::on_write(int status) {
		auto& l = llae::app::get(get_loop()).lua();
        if (!l.native()) {
            m_cont.release();
            m_buffers.release();
            return;
        }
        m_buffers.reset(l);
		if (m_cont.valid()) {
			m_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			m_cont.reset(l);
		}
	}

    void write_lua_req::reset(lua::state& l) {
        m_buffers.reset(l);
        m_cont.reset(l);
    }

	bool write_lua_req::putm(lua::state& l,int base) {
        return m_buffers.putm(l,base);
	}

	int write_lua_req::write() {
		return start_write(m_buffers.get_buffers().data(),m_buffers.get_buffers().size());
	}

	write_buffer_req::write_buffer_req(stream_ptr&& s,buffer_base_ptr&& b) : write_req(std::move(s)),m_buffer(std::move(b)) {
	}
	write_buffer_req::~write_buffer_req() {
	}

	void write_buffer_req::on_write(int status) {
		m_buffer.reset();
		if (status < 0) {
			print_error(status);
		}
	}

	int write_buffer_req::write() {
		return start_write(m_buffer->get(),1);
	}


	

	shutdown_req::shutdown_req(stream_ptr&& s) : m_stream(std::move(s)) {
		attach(reinterpret_cast<uv_req_t*>(&m_shutdown));
	}
	shutdown_req::~shutdown_req() {
	}

	void shutdown_req::shutdown_cb(uv_shutdown_t* req, int status) {
		shutdown_req* self = static_cast<shutdown_req*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_shutdown(status);
		self->remove_ref();
	}

	

	int shutdown_req::shutdown() {
		add_ref();
		int r = uv_shutdown(&m_shutdown,m_stream->get_stream(),&shutdown_req::shutdown_cb);
		if (r < 0) {
			remove_ref();
		}
		return r;
	}

	shutdown_lua_req::shutdown_lua_req(stream_ptr&& s,lua::ref&& cont) : shutdown_req(std::move(s)), m_cont(std::move(cont)) {
	}

	void shutdown_lua_req::reset(lua::state& l) {
		if (m_cont.valid()) {
			m_cont.reset(l);
		}
	}

	void shutdown_lua_req::on_shutdown(int status) {
		auto& l = llae::app::get(get_loop()).lua();
		if (m_cont.valid()) {
			if (!l.native()) {
				m_cont.release();
				return;
			}
			m_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			m_cont.reset(l);
		} 
	}

	stream::stream() {
	}

	stream::~stream() {
	}

    

	void stream::on_closed() {
		//LLAE_DIAG(std::cout << "stream::on_closed" << std::endl;)
        readable_stream::on_closed();
        handle::on_closed();
	}

	uv::buffer_ptr readable_stream::get_read_buffer(size_t size) {
		while (!m_read_buffers.empty()) {
			auto res = std::move(m_read_buffers.back());
			m_read_buffers.pop_back();
			if (res && res->get_capacity() >= size) {
				res->set_len(res->get_capacity());
				return res;
			}
		}
		return buffer::alloc(size);
	}

	void stream::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		stream* self = static_cast<stream*>(handle->data);
		auto b = self->get_read_buffer(suggested_size);
		buf->base = static_cast<char*>(b->get_base());
        buf->len = b->get_capacity();
        b->add_ref();
	}
	void stream::read_cb(uv_stream_t* s, ssize_t nread, const uv_buf_t* buf) {
		stream* self = static_cast<stream*>(s->data);
        buffer_ptr b{buffer::get(buf->base)};
        if (b) {
            b->remove_ref();
        }
        self->consume_read(nread,b);
        if (b) {
            self->add_read_buffer(std::move(b));
        }
	}
	void readable_stream::add_read_buffer(uv::buffer_ptr&& b) {
        if (!b) return;
		m_read_buffers.emplace_back(std::move(b));
	}

   

	class lua_read_consumer_base : public stream_read_consumer {
	protected:
		static bool push_read( lua::state& toth, ssize_t nread,  buffer_ptr& buffer ) {
			if (nread > 0) {
                buffer->set_len(nread);
                lua::push(toth,std::move(buffer));
                toth.pushnil();
            } else if (nread == UV_EOF) {
                toth.pushnil();
                toth.pushnil();
            } else if (nread < 0) {
                toth.pushnil();
                uv::push_error(toth,nread);
            } else { // == 0
                return false;
            }
            return true;
		}
	};
    class readable_stream::lua_read_consumer : public lua_read_consumer_base {
    private:
        lua::ref m_read_cont;
        std::vector<uv::buffer_ptr> m_readed;
        size_t m_readed_size = 0;
        ssize_t m_read_status = 0;
        bool m_read_status_consumed = false;
    public:
        lua_read_consumer() {}
        
        void on_closed() {
            m_read_cont.release();
        }
        
        void reset(lua::state& l) {
        	if (m_read_cont.valid()) {
        		m_read_cont.reset(l);
        	}
        }
        bool is_active() const {
            return m_read_cont.valid();
        }
        void start(lua::ref&& cont) {
            assert(!m_read_cont.valid());
            m_read_cont = std::move(cont);
            m_read_status_consumed = false;
        }
        bool try_read(lua::state& l) {
            if (!m_readed.empty()) {
                //std::cout << this << " pop stored buffer " << m_readed.front()->get_base() << " " << m_readed.front()->get_len() << std::endl;
                m_readed_size -= m_readed.front()->get_len();
                lua::push(l,std::move(m_readed.front()));
                l.pushnil();
                m_readed.erase(m_readed.begin());
                return true;
            }
            if (!m_read_status_consumed) {
                m_read_status_consumed = true;
                if (m_read_status==UV_EOF) {
                    l.pushnil();
                    l.pushnil();
                    return true;
                }
                if (m_read_status<0) {
                    l.pushnil();
                    uv::push_error(l,m_read_status);
                    return true;
                }
            }
            return false;
        }
        virtual bool on_read(readable_stream* s,
                             ssize_t nread,
                             buffer_ptr& buffer) override final {
            auto& l = s->get_lua();
            if (!l.native()) {
                m_read_cont.release();
                return true;
            }
            if (m_read_cont.valid()) {
                l.checkstack(2);
                m_read_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                if (!push_read(toth,nread,buffer)) {
                	return false;
                }
                
                lua::ref ref(std::move(m_read_cont));
                
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    ref.reset(l);
                    llae::app::show_error(toth,s);
                    return true;
                }
                ref.reset(l);
            } else if (nread > 0) {
                buffer->set_len(nread);
                m_readed_size += nread;
                if (m_readed.empty()) {
                    //std::cout << this << " push buffer " << buffer->get_base() << " " << buffer->get_len() << std::endl;
                    m_readed.emplace_back(std::move(buffer));
                    
                } else {
                    auto& last{m_readed.back()};
                    size_t last_tail = last->get_capacity() - last->get_len();
                    if (last_tail >= nread) {
                        //std::cout << this << " add buffer " << buffer->get_base() << " " << buffer->get_len() << std::endl;
                        ::memcpy(last->get_end(),buffer->get_base(),nread);
                        last->set_len(last->get_len()+buffer->get_len());
                        buffer->set_len(buffer->get_capacity());
                        s->add_read_buffer(std::move(buffer));
                    } else {
                        //std::cout << this << " push buffer " << buffer->get_base() << " " << buffer->get_len() << std::endl;
                        m_readed.emplace_back(std::move(buffer));
                    }
                }
                if (m_readed_size >= 1024*1024) {
                    return true;
                }
            } else if (nread != 0) {
                m_read_status_consumed = false;
                m_read_status = nread;
                return true;
            }
            return false;
        }
        virtual void on_stop_read(readable_stream* s) override final {
            auto& l = s->get_lua();
            if (!l.native()) {
                m_read_cont.release();
            } else if (m_read_cont.valid()) {
                l.checkstack(2);
                m_read_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                if (!try_read(toth)) {
                    toth.pushnil();
                    toth.pushnil();
                }
                lua::ref ref(std::move(m_read_cont));
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    ref.reset(l);
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
            }
        }
    };

    readable_stream::readable_stream() {
        
    }

    readable_stream::~readable_stream() {
        
    }

    void readable_stream::on_closed() {
        set_closed();
        stop_read();
        if (m_lua_reader) {
            m_lua_reader->on_closed();
            m_lua_reader.reset();
        }
    }

	void readable_stream::consume_read(ssize_t nread,  buffer_ptr& buffer) {
        if (m_read_consumer) {
            auto consumer = m_read_consumer;
            bool res = consumer->on_read(this, nread, buffer);
            if (res) {
                if (consumer.get() == m_read_consumer.get()) {
                    stop_read();
                }
            }
        } else {
            LLAE_DIAG_ERROR(std::cout << "read without consumer" << std::endl;)
            stop_read();
        }
    }

    void readable_stream::stop_read() {
        stream_read_consumer_ptr consumer = std::move(m_read_consumer);
        if (consumer) {
            consumer->on_stop_read(this);
            unhold_ref();
        }
    }

    
    lua::state& stream::get_lua() {
        return llae::app::get(get_stream()->loop).lua();
    }
        
    void stream::stop_read() {
        //std::cout << "stop read " << this << std::endl;
        uv_read_stop(get_stream());
        readable_stream::stop_read();
    }

	lua::multiret readable_stream::read(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::read is async");
			return {2};
		}
		if (m_read_consumer && (m_read_consumer.get()!=m_lua_reader.get() || m_lua_reader->is_active()) ) {
			l.pushnil();
			l.pushstring("stream::read already read");
			return {2};
		}
		if (m_closed) {
			l.pushnil();
			l.pushnil();
			return {2};
		}
        {
            if (m_lua_reader) {
                if (m_lua_reader->try_read(l)) {
                    return {2};
                }
            } else {
                m_lua_reader.reset(new lua_read_consumer());
            }
        }
		{
			l.pushthread();
            lua::ref read_cont;
			read_cont.set(l);
            m_lua_reader->start(std::move(read_cont));
            if (!m_read_consumer) {
                int res = start_read(m_lua_reader);
                if (res < 0) {
                    m_lua_reader->reset(l);
                    l.pushnil();
                    uv::push_error(l,res);
                    return {2};
                }
            }
		}
		l.yield(0);
		return {0};
	}

    int readable_stream::start_read( const stream_read_consumer_ptr& consumer ) {
        if (m_read_consumer) {
            return -1;
        }
        if (is_closed()) {
            return -2;
        }
        m_read_consumer = consumer;
        hold_ref();
        return 0;
    }

    int stream::start_read( const stream_read_consumer_ptr& consumer ) {
        auto res = readable_stream::start_read( consumer );
        if (res < 0) {
            return res;
        }
        //std::cout << "stream start_read add_ref" << std::endl;
        //std::cout << "start read " << this << std::endl;
        res = uv_read_start(get_stream(), &stream::alloc_cb, &stream::read_cb);
        if (res < 0) {
            readable_stream::stop_read();
        }
        return res;
    }

	lua::multiret stream::write(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::write is async");
			return {2};
		}
        {
			l.pushthread();
			lua::ref write_cont;
			write_cont.set(l);

			common::intrusive_ptr<write_lua_req> req{new write_lua_req(stream_ptr(this),std::move(write_cont))};
		
		  	if (!req->putm(l,2)) {
                req->reset(l);
                l.pushnil();
                l.pushstring("stream::write invalid data");
                return {2};
            }
            if (req->empty()) {
                req->reset(l);
                l.pushboolean(true);
                return {1};
            }

			int r = req->write();
			if (r < 0) {
				req->reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}


	bool stream::write(buffer_base_ptr&& buf) {
		common::intrusive_ptr<write_buffer_req> req{new write_buffer_req(stream_ptr(this),std::move(buf))};
		int r = req->write();
		if (r < 0) {
			print_error(r);
			return false;
		} 
		return true;
	}

	lua::multiret stream::send(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::send is async");
			return {2};
		}
		{
			auto f = lua::stack<common::intrusive_ptr<file> >::get(l,2);
			if (!f) {
				l.argerror(2,"uv::file expected");
			}
			l.pushthread();
			lua::ref write_cont;
			write_cont.set(l);

			common::intrusive_ptr<write_file_pipe> req{new write_file_pipe(stream_ptr(this),
				std::move(f),std::move(write_cont))};
			
			int r = req->start();
			if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	lua::multiret stream::shutdown(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::shutdown is async");
			return {2};
		}
		if (is_closed()) {
			l.pushnil();
			l.pushstring("stream::shutdown stream closed");
			return {2};
		}
        {
            stop_read();
        }
		{
			l.pushthread();
			lua::ref shutdown_cont;
			shutdown_cont.set(l);
		
			common::intrusive_ptr<shutdown_lua_req> req{new shutdown_lua_req(stream_ptr(this),std::move(shutdown_cont))};
		
			int r = req->shutdown();
			if (r < 0) {
				req->reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			}
		} 
		l.yield(0);
		return {0};
	}

	void stream::close() {
        stop_read();
		handle::close();
	}

	void stream::lbind(lua::state& l) {
		lua::bind::function(l,"read",&stream::read);
		lua::bind::function(l,"write",static_cast<lua::multiret(stream::*)(lua::state&)>(&stream::write));
		lua::bind::function(l,"send",&stream::send);
		lua::bind::function(l,"shutdown",&stream::shutdown);
		lua::bind::function(l,"close",&stream::close);
        lua::bind::function(l,"stop_read",&stream::stop_read);
        lua::bind::function(l,"add_read_buffer",&stream::add_read_buffer);
	}
}
