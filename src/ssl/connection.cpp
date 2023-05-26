#include "connection.h"
#include "llae/app.h"
#include "uv/luv.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "crypto/crypto.h"
#include <iostream>
#include <mbedtls/debug.h>


META_OBJECT_INFO(ssl::connection,meta::object)

namespace ssl {
    using crypto::push_error;

	connection::connection( ctx_ptr&& ctx, uv::stream_ptr&& stream ) : m_ctx(std::move(ctx)), m_stream(std::move(stream)) {
        
        mbedtls_ssl_init( &m_ssl );
		mbedtls_ssl_config_init( &m_conf );
        mbedtls_ssl_session_init( &m_session );
		m_write_req.data = this;
		m_write_buf = uv_buf_init(m_write_data_buf,CONN_BUFFER_SIZE);
        //std::cout << "ssl::connection::connection" << std::endl;
	}

	connection::~connection() {
        mbedtls_ssl_session_free( &m_session );
		mbedtls_ssl_free( &m_ssl );
		mbedtls_ssl_config_free( &m_conf );
        if (m_stream) {
            m_stream->close();
        }
        //std::cout << "ssl::connection::~connection" << std::endl;
	}

	void connection::dbg_cb(void *vctx, int level,
                      const char *file, int line,
                      const char *str) {
		//connection* self = static_cast<connection*>(vctx);
		std::cout << "ssl: " << file << ":" << line << " " << str << std::endl;
			
	}

	int connection::send_cb( void *vctx,
                                const unsigned char *buf,
                                size_t len ) {
		connection* self = static_cast<connection*>(vctx);
		return self->ssl_send(buf,len);
	}
	int connection::recv_cb( void *vctx,
                            unsigned char *buf,
                            size_t len ) {
		connection* self = static_cast<connection*>(vctx);
		return self->ssl_recv(buf,len);
	}

	void connection::write_cb(uv_write_t* req, int status) {
		connection* self = static_cast<connection*>(req->data);
		self->on_write_complete(status);
	}

	lua::multiret connection::configure(lua::state& l) {
        int ret = mbedtls_ssl_config_defaults( &m_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT );
		if (ret != 0) {
			l.pushnil();
            ::ssl::push_error(l,"mbedtls_ssl_config_defaults failed, code:%d, %s",ret);
			return {2};
		}
		mbedtls_ssl_conf_dbg( &m_conf, &connection::dbg_cb, this );

		ret = m_ctx->configure(&m_conf);
		if (ret != 0) {
			l.pushnil();
            ::ssl::push_error(l,"configure failed, code:%d, %s",ret);
			return {2};
		}

		ret = mbedtls_ssl_setup( &m_ssl, &m_conf );
		if (ret != 0) {
			l.pushnil();
            ::ssl::push_error(l,"mbedtls_ssl_setup failed, code:%d, %s",ret);
			return {2};
		}

		mbedtls_ssl_set_bio( &m_ssl, this, &connection::send_cb, &connection::recv_cb, NULL );
		l.pushboolean(true);
		return {1};
	}

	lua::multiret connection::set_host(lua::state& l,const char* host) {
		int ret = mbedtls_ssl_set_hostname( &m_ssl, host );
		if (ret != 0) {
			l.pushnil();
            ::ssl::push_error(l,"mbedtls_ssl_set_hostname failed, code:%d, %s",ret);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void connection::push_error(lua::state& l) {
		if (m_uv_error) {
			uv::push_error(l,m_uv_error);
		} else if (m_ssl_error) {
            ::ssl::push_error(l,"ssl error code:%d, %s",m_ssl_error);
		} else {
			l.pushstring("unknown");
		}
	}

	lua::multiret connection::handshake(lua::state& l) {
        if (is_error()) {
            l.pushnil();
            l.pushstring("connection::handshake is error");
            return {2};
        }
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("connection::handshake is async");
			return {2};
		}
		if (m_state != S_NONE) {
			l.pushnil();
			l.pushstring("stream::read invalid state");
			return {2};
		}
		{
			l.pushthread();
			m_write_cont.set(l);

            //std::cout << "begin handshake" << std::endl;
            begin_op("HANDSHAKE");
			if (!do_handshake()) {
				m_write_cont.reset(l);
				l.pushnil();
				push_error(l);
                end_op("HANDSHAKE");
				return {2};
            } else if (m_state == S_CONNECTED) {
                m_write_cont.reset(l);
                end_op("HANDSHAKE");
                l.pushboolean(true);
                return {1};
            }
		}
		l.yield(0);
		return {0};
	}

	void connection::on_write_complete(int status) {
        //std::cout << "on_write_complete: " << status << std::endl;
		m_write_active = false;
		if (status < 0 && m_uv_error == 0) {
            //std::cout << "on_write_complete error " << status << std::endl;
			m_uv_error = status;
		}
		do_continue();
	}

	bool connection::do_handshake() {
        //std::cout << "do_handshake " << std::endl;
        
		m_state = S_HANDSHAKE;
		int ret = mbedtls_ssl_handshake( &m_ssl );
        if (ret == 0) {
            //std::cout << "do_handshake S_CONNECTED" << std::endl;
            m_state = S_CONNECTED;
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            //std::cout << "do_handshake error" << std::endl;
			m_ssl_error = ret;
            m_stream->stop_read();
			return false;
		}
		
		return true;
	}

    lua::state& connection::get_lua() {
        return llae::app::get(m_stream->get_stream()->loop).lua();
    }

    bool connection::do_write() {
        //std::cout << "do_write " << m_write_buffers.get_write_size() << std::endl;
        
        m_state = S_WRITE;
        auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
        while (!m_write_buffers.empty()) {
            //std::cout << "mbedtls_ssl_write " << m_write_buffers.get_write_size() << std::endl;
            int ret = mbedtls_ssl_write( &m_ssl, m_write_buffers.get_write_ptr(), m_write_buffers.get_write_size());
            if (ret > 0) {
                m_write_buffers.consume(l,ret);
                continue;
            }
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                m_write_buffers.reset(l);
                m_ssl_error = ret;
                m_stream->stop_read();
                //std::cout << "mbedtls_ssl_write failed: " << ret << std::endl;
                return false;
            }
            return true;
        }
        //std::cout << "do_write S_CONNECTED" << std::endl;
        m_state = S_CONNECTED;
        return true;
    }

    class connection::shutdown_stream_req : public uv::shutdown_req {
    private:
        common::intrusive_ptr<connection> m_connection;
    protected:
        virtual void on_shutdown(int status) {
            m_connection->on_shutdown(status);
            m_connection.reset();
        }
    public:
        shutdown_stream_req(common::intrusive_ptr<connection>&& connection) : uv::shutdown_req(
            common::intrusive_ptr<uv::stream>(connection->m_stream)),
            m_connection(connection) {

            };
    };

    void connection::on_shutdown(int status) {
        //std::cout << "shutdown stream completed: " << status << std::endl;
        if (status < 0) {
            m_uv_error = status;
        }
        
        m_state = S_CLOSED;
        finish_status("SHUTDOWN");
        
        m_stream->close();
        m_stream.reset();
    }

    bool connection::do_shutdown_stream() {
        m_stream->stop_read();
        common::intrusive_ptr<shutdown_stream_req> req(new shutdown_stream_req(common::intrusive_ptr<connection>(this)));
        int st = req->shutdown();
        if (st<0) {
            m_uv_error = st;
            return false;
        } else {
           return true;
        }
    }

    bool connection::do_shutdown() {
        m_state = S_SHUTDOWN;
        int ret = mbedtls_ssl_close_notify(&m_ssl);
        if (ret == 0) {
            //std::cout << "shutdown stream" << std::endl;
            m_state = S_SHUTDOWN_STREAM;
            if (m_write_active) {
                return true;
            }
            return do_shutdown_stream();
        }
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            m_ssl_error = ret;
            m_stream->stop_read();
            //std::cout << "mbedtls_ssl_close_notify failed: " << ret << std::endl;
            return false;
        }
        return true;
    }


	void connection::finish_status(const char* state) {
        //std::cout << "finish_status " << m_uv_error << " / " << m_ssl_error << std::endl;
        if (!m_stream) {
            m_write_cont.release();
            m_write_buffers.release();
            return;
        }
		auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
        if (!l.native()) {
            m_write_cont.release();
            m_write_buffers.release();
            return;
        }
        if (m_write_cont.valid()) {
            m_write_cont.push(l);
			auto toth = l.tothread(-1);
            m_write_cont.reset(l);
            int args;
			if (is_error()) {
				toth.pushnil();
				push_error(toth);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
            common::intrusive_ptr<connection> lock(std::move(m_active_op_lock));
            lock->end_op(state);
			auto s = toth.resume(l,args);
            if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
                m_stream->stop_read();
			}
			l.pop(1);// thread
        } else {
            //std::cout << "finish_status without cont" << std::endl;
            end_op(state);
        }
        
	}

	void connection::do_continue() {
        process_read();
		if (m_state == S_HANDSHAKE) {
            //std::cout << "do_continue S_HANDSHAKE" << std::endl;
			if (m_uv_error || m_ssl_error || !do_handshake()) {
				finish_status("HANDSHAKE");
			} else if (m_state == S_CONNECTED) {
				finish_status("HANDSHAKE");
			} else {
				// continue handshake
				return;
			}
		} else if (m_state == S_WRITE) {
            //std::cout << "do_continue S_WRITE" << std::endl;
            if (m_uv_error || m_ssl_error || !do_write()) {
                finish_status("WRITE");
            } else if (m_state == S_CONNECTED) {
                finish_status("WRITE");
            } else {
                // continue write
                return;
            }
        } else if (m_state == S_SHUTDOWN) {
            //std::cout << "do_continue S_SHUTDOWN" << std::endl;
            if (m_uv_error || m_ssl_error || !do_shutdown()) {
                finish_status("SHUTDOWN");
            } else if (m_state == S_CLOSED) {
                finish_status("SHUTDOWN");
            } else {
                // continue close
                return;
            }
        } else if (m_state == S_SHUTDOWN_STREAM) {
            do_shutdown_stream();
        }
	}

	int connection::ssl_send( const unsigned char *buf,
                                size_t len ) {
		if (m_write_active) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}
        if (m_state == S_SHUTDOWN_STREAM ||
            m_state == S_CLOSED) {
            std::cout << "ssl_send closed" << std::endl;
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        }

        //std::cout << "ssl_send: " << len << std::endl;
		
		size_t count = len;
		if (count > CONN_BUFFER_SIZE) {
			count = CONN_BUFFER_SIZE;
		}
		memcpy(m_write_data_buf,buf,count);
		m_write_buf = uv_buf_init(m_write_data_buf,static_cast<unsigned int>(count));

		m_write_active = true;
		int r = uv_write(&m_write_req,m_stream->get_stream(),&m_write_buf,1,&connection::write_cb);
		if (r < 0) {
            //std::cout << "ssl_send failed" << std::endl;
			m_write_active = false;
			return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
		} 
		return int(count);
	}

    bool connection::on_read(uv::readable_stream* s,ssize_t nread, const uv::buffer_ptr&& buffer) {
        //std::cout << "on_read " << nread << std::endl;
        if (nread > 0) {
            m_readed_data.push_back({size_t(nread),0,std::move(buffer)});
            do_continue();
        } else if (nread == UV_EOF) {
            m_read_state = RS_EOF;
            do_continue();
        } else if (nread < 0) {
            m_read_state = RS_ERROR;
            std::cout << "on_read error " << nread << std::endl;
            m_uv_error = int(nread);
            do_continue();
        }
        return is_error();
    }

    void connection::on_stream_closed(uv::readable_stream* s) {
        // @todo
        //LLAE_DIAG(std::cout << "on_stream_closed" << std::endl;)
        m_state = S_CLOSED;
        m_read_state = RS_EOF;
        do_continue();
    }

	int connection::ssl_recv( unsigned char *buf,
                                size_t len) {
        //std::cout << "ssl_recv " << len << std::endl;
        if (m_read_state == RS_NONE) {
            m_read_state = RS_ACTIVE;
            //std::cout << "start read " << std::endl;
            int r = m_stream->start_read(uv::stream_read_consumer_ptr(this));
            if (r < 0) {
                return MBEDTLS_ERR_SSL_WANT_READ;
            }
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        if (m_ssl_error!=0) {
            return m_ssl_error;
        }
        if (m_uv_error) {
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        }
        if (m_readed_data.empty()) {
            if (m_read_state == RS_EOF) {
                return 0;
            }
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        int readed = 0;
        while (!m_readed_data.empty() && len>0) {
            auto& d(m_readed_data.front());
            size_t psize = d.size - d.readded;
            size_t rsize = psize;
            if (rsize > len) {
                rsize = len;
            }
            //std::cout << "ssl_recv readed " << rsize << std::endl;
            ::memcpy(buf, static_cast<const unsigned char*>(d.data->get_base())+d.readded, rsize);
            buf += rsize;
            len -= rsize;
            readed += rsize;
            d.readded += rsize;
            if (d.readded == d.size)
                m_readed_data.pop_front();
        }
		return readed;
	}



    lua::multiret connection::write(lua::state& l) {
        if (is_error()) {
            l.pushnil();
            l.pushstring("connection::write is error: ");
            push_error(l);
            l.concat(2);
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("connection::write is async");
            return {2};
        }
        if (m_write_cont.valid()) {
            l.pushnil();
            l.pushstring("connection::write async not completed");
            return {2};
        }
        {
            l.pushthread();
            m_write_cont.set(l);
            
            l.pushvalue(2);
            m_write_buffers.put(l);
            
            begin_op("WRITE");
            if (!do_write()) {
                m_write_cont.reset(l);
                l.pushnil();
                push_error(l);
                end_op("WRITE");
                return {2};
            }
            if (m_state == S_CONNECTED) {
                // all writed
                l.pushboolean(true);
                return {1};
            }
        }
        l.yield(0);
        return {0};
    }

    void connection::begin_op(const char* op) {
        if (m_active_op) {
            std::cerr << "begin op with active: " << op << "/" << m_active_op << std::endl;
        }
        m_active_op = op;
        m_active_op_lock.reset(this);
    }
    void connection::end_op(const char* op) {
        if (!m_active_op || strcmp(m_active_op, op)!=0) {
            std::cerr << "finish status with different op: " << op << "/" << (m_active_op?m_active_op:"null") << std::endl;
        }
        m_active_op = nullptr;
        m_active_op_lock.reset();
    }

    template <typename Handle>
    bool connection::do_read(Handle handle) {
        while (true) {
            if (!m_read_data_buf) {
                m_read_data_buf = uv::buffer::alloc(CONN_BUFFER_SIZE);
            }
            unsigned char* data = reinterpret_cast<unsigned char*>(m_read_data_buf->get_base());
            int r = mbedtls_ssl_read(&m_ssl, data, CONN_BUFFER_SIZE);
            if (r > 0) {
                m_read_data_buf->set_len(r);
                handle(r,std::move(m_read_data_buf));
                return true;
            } else if( r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                m_state = S_CLOSED;
                m_stream->stop_read();
                m_stream->close();
                handle(UV_EOF,std::move(m_read_data_buf));
                return true;
            } else if( r == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
                auto ret = mbedtls_ssl_get_session(&m_ssl, &m_session);
                if (ret != 0) {
                    m_stream->stop_read();
                    m_ssl_error = ret;
                    handle(-1,std::move(m_read_data_buf));
                    return true;
                }
                if (m_read_state == RS_EOF) {
                    if (m_readed_data.empty()) {
                        m_stream->stop_read();
                        m_ssl_error = MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET;
                        handle(-1,std::move(m_read_data_buf));
                        return true;
                    }
                }
                continue;
            } else if (r != MBEDTLS_ERR_SSL_WANT_READ && r!= MBEDTLS_ERR_SSL_WANT_WRITE) {
                m_stream->stop_read();
                m_ssl_error = r;
                handle(-1,std::move(m_read_data_buf));
                return true;
            }
            if (!m_readed_data.empty() && r==MBEDTLS_ERR_SSL_WANT_READ)
                continue;
            if (m_read_state == RS_EOF && r==MBEDTLS_ERR_SSL_WANT_READ) {
                handle(UV_EOF,std::move(m_read_data_buf));
                return true;
            }
            return false;
        }
    }
            
    void connection::process_read() {
        if (!is_read_active())
            return;
        do_read([this](int status,uv::buffer_ptr&& data){
            consume_read(status,std::move(data));
        });
    }
    
    lua::multiret connection::close(lua::state& l) {
        if (m_stream) {
            m_stream->close();
        }
        return {0};
    }

    lua::multiret connection::shutdown(lua::state& l) {
        if (is_error()) {
            l.pushnil();
            l.pushstring("connection::shutdown is error");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("connection::shutdown is async");
            return {2};
        }
        if (m_write_cont.valid()) {
            l.pushnil();
            l.pushstring("connection::shutdown async not completed");
            return {2};
        }
        {
            if (m_state != S_CONNECTED) {
                l.pushnil();
                l.pushstring("connection::shutdown invalid state");
                return {2};
            }
            
            l.pushthread();
            m_write_cont.set(l);
            
            begin_op("SHUTDOWN");
            if (!do_shutdown()) {
                m_write_cont.reset(l);
                l.pushnil();
                push_error(l);
                end_op("SHUTDOWN");
                return {2};
            }
            if (m_state == S_CLOSED) {
                // all writed
                l.pushboolean(true);
                return {1};
            }
            
        }
        l.yield(0);
        return {0};
    }

    int connection::start_read( const uv::stream_read_consumer_ptr& consumer ) {
        auto res = uv::readable_stream::start_read(consumer);
        return res;
    }

    lua::multiret connection::read(lua::state& l) {
        if (is_error()) {
            l.pushnil();
            l.pushstring("connection::read is error");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("connection::read is async");
            return {2};
        }
        if (is_read_active()) {
            l.pushnil();
            l.pushstring("connection::read is active");
            return {2};
        }
        if (do_read([&l](int nread,uv::buffer_ptr&& buffer){
            if (nread >= 0) {
                l.pushlstring(static_cast<const char*>(buffer->get_base()),nread);
                l.pushnil();
            } else if (nread == UV_EOF) {
                l.pushnil();
                l.pushnil();
            } else if (nread < 0) {
                l.pushnil();
                uv::push_error(l,nread);
            } 
        })) {
            return {2};
        }
        return readable_stream::read(l);
    }

    void connection::stop_read() {
        uv::readable_stream::stop_read();
    }
	void connection::lbind(lua::state& l) {
        
        lua::bind::constructor<connection, lua::check<ctx_ptr>, lua::check<uv::stream_ptr> >(l);
		lua::bind::function(l,"configure",&connection::configure);
		lua::bind::function(l,"set_host",&connection::set_host);
		lua::bind::function(l,"handshake",&connection::handshake);
        lua::bind::function(l,"write",&connection::write);
        lua::bind::function(l,"read",&connection::read);
        lua::bind::function(l,"close",&connection::close);
        lua::bind::function(l,"shutdown",&connection::shutdown);
        lua::bind::function(l,"stop_read",&connection::stop_read);
	}
}
