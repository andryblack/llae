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

	bool write_lua_req::put(lua::state& l) {
        return m_buffers.put(l);
	}

	int write_lua_req::write() {
		return start_write(m_buffers.get_buffers().data(),m_buffers.get_buffers().size());
	}

	write_buffer_req::write_buffer_req(stream_ptr&& s,buffer_ptr&& b) : write_req(std::move(s)),m_buffer(std::move(b)) {
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

	void shutdown_lua_req::on_shutdown(int status) {
		auto& l = llae::app::get(get_loop()).lua();
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

	stream::stream() {
	}

	stream::~stream() {
	}

	void stream::on_closed() {
		//LLAE_DIAG(std::cout << "stream::on_closed" << std::endl;)
		m_closed = true;
        if (m_read_consumer) {
            m_read_consumer->on_stream_closed(this);
            m_read_consumer.reset();
        }
        handle::on_closed();
	}

	void stream::alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        auto b = buffer::alloc(suggested_size);
        buf->base = static_cast<char*>(b->get_base());
        buf->len = b->get_len();
        b->add_ref();
	}
	void stream::read_cb(uv_stream_t* s, ssize_t nread, const uv_buf_t* buf) {
		stream* self = static_cast<stream*>(s->data);
        auto b = buffer::get(buf->base);
        if (self->on_read(nread,buffer_ptr(b))) {
            //std::cout << "stream read_cb remove_ref" << std::endl;
            self->remove_ref();
		}
        if (b) {
            b->remove_ref();
        }
	}

    class lua_read_consumer : public stream_read_consumer {
    private:
        lua::ref m_read_cont;
    public:
        lua_read_consumer(lua::ref && cont) : m_read_cont(std::move(cont)) {}
        virtual bool on_read(stream* s,
                             ssize_t nread,
                             const buffer_ptr&& buffer) override final {
            auto& l = llae::app::get(s->get_stream()->loop).lua();
            if (m_read_cont.valid()) {
                m_read_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                if (nread > 0) {
                    toth.pushlstring(static_cast<const char*>(buffer->get_base()),nread);
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
                lua::ref ref(std::move(m_read_cont));
                s->stop_read();
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
            } else {
                s->stop_read();
            }
            return true;
        }
        void on_stream_closed(stream* s) override final {
        	auto& l = llae::app::get(s->get_stream()->loop).lua();
            if (!l.native()) {
                m_read_cont.release();
            }
        	if (m_read_cont.valid()) {
        		m_read_cont.push(l);
                auto toth = l.tothread(-1);
                l.pop(1);// thread
                toth.pushnil();
                toth.pushnil();
                lua::ref ref(std::move(m_read_cont));
                auto s = toth.resume(l,2);
                if (s != lua::status::ok && s != lua::status::yield) {
                    llae::app::show_error(toth,s);
                }
                ref.reset(l);
        	}
        }
    };

	bool stream::on_read(ssize_t nread, const buffer_ptr&& buffer) {
        if (m_read_consumer) {
            auto consumer = std::move(m_read_consumer);
            bool res = consumer->on_read(this,
                                                nread, std::move(buffer));
            if (!res) {
                m_read_consumer = std::move(consumer);
            }
            return res;
        } else {
            LLAE_DIAG(std::cout << "read without consumer" << std::endl;)
            stop_read();
        }
        return true;
    }
        
    void stream::stop_read() {
        uv_read_stop(get_stream());
        if (m_read_consumer) {
        	m_read_consumer->on_stop_read(this);
        }
        m_read_consumer.reset();
    }

	lua::multiret stream::read(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("stream::read is async");
			return {2};
		}
		if (m_read_consumer) {
			l.pushnil();
			l.pushstring("stream::read already read");
			return {2};
		}
		if (m_closed) {
			l.pushnil();
			l.pushstring("stream::read closed");
			return {2};
		}
		{
			l.pushthread();
            lua::ref read_cont;
			read_cont.set(l);

            common::intrusive_ptr<lua_read_consumer> consume(new lua_read_consumer(std::move(read_cont)));
            int res = start_read(consume);
            if (res < 0) {
                l.pushnil();
                uv::push_error(l,res);
                return {2};
            }
			
		}
		l.yield(0);
		return {0};
	}

    int stream::start_read( const stream_read_consumer_ptr& consumer ) {
        if (m_read_consumer) {
            return -1;
        }
        m_read_consumer = consumer;
        //std::cout << "stream start_read add_ref" << std::endl;
        add_ref();
        int res = uv_read_start(get_stream(), &stream::alloc_cb, &stream::read_cb);
        if (res < 0) {
            m_read_consumer.reset();
            //std::cout << "stream start_read error remove_ref" << std::endl;
            remove_ref();
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
		
			l.pushvalue(2);
            if (!req->put(l)) {
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
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

	bool stream::write(buffer_ptr&& buf) {
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
		
		{
			l.pushthread();
			lua::ref shutdown_cont;
			shutdown_cont.set(l);
		
			common::intrusive_ptr<shutdown_req> req{new shutdown_lua_req(stream_ptr(this),std::move(shutdown_cont))};
		
			int r = req->shutdown();
			if (r < 0) {
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			}
		} 
		l.yield(0);
		return {0};
	}

	void stream::close() {
		handle::close();
	}

	void stream::lbind(lua::state& l) {
		lua::bind::function(l,"read",&stream::read);
		lua::bind::function(l,"write",&stream::write);
		lua::bind::function(l,"send",&stream::send);
		lua::bind::function(l,"shutdown",&stream::shutdown);
		lua::bind::function(l,"close",&stream::close);
        lua::bind::function(l,"stop_read",&stream::stop_read);
	}
}
