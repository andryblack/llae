#include "connection.h"
#include "llae/app.h"
#include "uv/luv.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <iostream>
#include <mbedtls/debug.h>

META_OBJECT_INFO(ssl::connection,meta::object)

namespace ssl {

	connection::connection( const ctx_ptr& ctx, const uv::stream_ptr& stream ) : m_ctx(ctx), m_stream(stream) {
        mbedtls_ssl_init( &m_ssl );
		mbedtls_ssl_config_init( &m_conf );
		m_write_req.data = this;
		m_write_buf = uv_buf_init(m_write_data_buf,CONN_BUFFER_SIZE);
	}

	connection::~connection() {
		mbedtls_ssl_free( &m_ssl );
		mbedtls_ssl_config_free( &m_conf );
        if (m_stream) {
            m_stream->close();
        }
	}

	void connection::dbg_cb(void *vctx, int level,
                      const char *file, int line,
                      const char *str) {
		connection* self = static_cast<connection*>(vctx);
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
			l.pushfstring("mbedtls_ssl_config_defaults failed, code:%d",ret);
			return {2};
		}
		mbedtls_ssl_conf_authmode( &m_conf, MBEDTLS_SSL_VERIFY_NONE );
		mbedtls_ssl_conf_dbg( &m_conf, &connection::dbg_cb, this );

		ret = m_ctx->configure(&m_conf);
		if (ret != 0) {
			l.pushnil();
			l.pushfstring("configure failed, code:%d",ret);
			return {2};
		}

		ret = mbedtls_ssl_setup( &m_ssl, &m_conf );
		if (ret != 0) {
			l.pushnil();
			l.pushfstring("mbedtls_ssl_setup failed, code:%d",ret);
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
			l.pushfstring("mbedtls_ssl_set_hostname failed, code:%d",ret);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void connection::push_error(lua::state& l) {
		if (m_uv_error) {
			uv::push_error(l,m_uv_error);
		} else if (m_ssl_error) {
			l.pushfstring("ssl error %d",m_ssl_error);
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
			m_cont.set(l);

			add_ref();
			if (!do_handshake()) {
				m_cont.reset(l);
				l.pushnil();
				push_error(l);
				remove_ref();
				return {2};
			}
		}
		l.yield(0);
		return {0};
	}

	void connection::on_write_complete(int status) {
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
			m_ssl_error = ret;
            m_stream->stop_read();
			return false;
		}
		
		return true;
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

    bool connection::do_read(lua::state& l) {
        //std::cout << "do_read " << std::endl;
        
        m_state = S_READ;
        unsigned char* data = reinterpret_cast<unsigned char*>(m_read_data_buf);
        int r = mbedtls_ssl_read(&m_ssl, data, CONN_BUFFER_SIZE);
        if (r >= 0) {
            m_state = S_CONNECTED;
            //std::cout << "do_read end, has data " << r << std::endl;
            if (r) {
                l.pushlstring(m_read_data_buf, r);
            } else {
                l.pushnil();
            }
            l.pushnil();
            return true;
        } else if (r != MBEDTLS_ERR_SSL_WANT_READ && r!= MBEDTLS_ERR_SSL_WANT_WRITE) {
            m_stream->stop_read();
            m_ssl_error = r;
            l.pushnil();
            push_error(l);
            return true;
        }
        //std::cout << "do_read continue" << std::endl;
        /* need read */
        return false;
    }

	void connection::finish_status() {
        //std::cout << "finish_status " << m_uv_error << " / " << m_ssl_error << std::endl;
		auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
        if (m_cont.valid()) {
			m_cont.push(l);
			auto toth = l.tothread(-1);
			m_cont.reset(l);
            int args;
			if (is_error()) {
				toth.pushnil();
				push_error(l);
				args = 2;
			} else {
				toth.pushboolean(true);
				args = 1;
			}
			auto s = toth.resume(l,args);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
                m_stream->stop_read();
			}
			l.pop(1);// thread
        } else {
            //std::cout << "finish_status without cont" << std::endl;
        }
		remove_ref();
	}

	void connection::do_continue() {
		if (m_state == S_HANDSHAKE) {
            //std::cout << "do_continue S_HANDSHAKE" << std::endl;
			if (m_uv_error || m_ssl_error || !do_handshake()) {
				finish_status();
			} else if (m_state == S_CONNECTED) {
				finish_status();
			} else {
				// continue handshake
				return;
			}
		} else if (m_state == S_WRITE) {
            //std::cout << "do_continue S_WRITE" << std::endl;
            if (m_uv_error || m_ssl_error || !do_write()) {
                finish_status();
            } else if (m_state == S_CONNECTED) {
                finish_status();
            } else {
                // continue write
                return;
            }
        } else if (m_state == S_READ) {
            //std::cout << "do_continue S_READ" << std::endl;
            if (m_uv_error || m_ssl_error) {
                finish_status();
            } else {
                auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
                m_cont.push(l);
                auto toth = l.tothread(-1);
                if (do_read(toth)) {
                    m_cont.reset(l);
                    auto s = toth.resume(l,2);
                    if (s != lua::status::ok && s != lua::status::yield) {
                        llae::app::show_error(toth,s);
                        m_stream->stop_read();
                    }
                    l.pop(1);// thread
                    remove_ref();
                } else {
                    l.pop(1);// thread
                }
            }
        }
	}

	int connection::ssl_send( const unsigned char *buf,
                                size_t len ) {
		if (m_write_active) {
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}
		
		size_t count = len;
		if (count > CONN_BUFFER_SIZE) {
			count = CONN_BUFFER_SIZE;
		}
		memcpy(m_write_data_buf,buf,count);
		m_write_buf = uv_buf_init(m_write_data_buf,count);

		m_write_active = true;
		int r = uv_write(&m_write_req,m_stream->get_stream(),&m_write_buf,1,&connection::write_cb);
		if (r < 0) {
			m_write_active = false;
			return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
		} 
		return count;
	}

    bool connection::on_read(uv::stream* s,ssize_t nread, const uv::buffer_ptr&& buffer) {
        //std::cout << "on_read " << nread << std::endl;
        if (nread > 0) {
            m_readed_data.push_back({size_t(nread),0,std::move(buffer)});
            do_continue();
        } else if (nread == UV_EOF) {
            m_read_state = RS_EOF;
            do_continue();
        } else if (nread < 0) {
            //std::cout << "on_read error " << nread << std::endl;
            m_uv_error = nread;
        }
        return is_error();
    }

    void connection::on_stream_closed(uv::stream* s) {
        // @todo
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

	int connection::lnew(lua_State* L) {
		lua::state l(L);
		auto c = lua::stack<ctx_ptr>::get(l,2);
		auto stream = lua::stack<uv::stream_ptr>::get(l,3);
		common::intrusive_ptr<connection> conn{new connection(c,stream)};
		lua::push(l,std::move(conn));
		return 1;
	}


    lua::multiret connection::write(lua::state& l) {
        if (is_error()) {
            l.pushnil();
            l.pushstring("connection::write is error");
            return {2};
        }
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("connection::write is async");
            return {2};
        }
        if (m_cont.valid()) {
            l.pushnil();
            l.pushstring("connection::write async not completed");
            return {2};
        }
        {
            l.pushthread();
            m_cont.set(l);
            
            l.pushvalue(2);
            m_write_buffers.put(l);
            
            add_ref();
            if (!do_write()) {
                m_cont.reset(l);
                l.pushnil();
                push_error(l);
                remove_ref();
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
        if (m_cont.valid()) {
            l.pushnil();
            l.pushstring("connection::read async not completed");
            return {2};
        }
        {
            
            
            if (do_read(l)) {
                return {2};
            }
            add_ref();
            l.pushthread();
            m_cont.set(l);
        }
        l.yield(0);
        return {0};
    }

    lua::multiret connection::close(lua::state& l) {
        mbedtls_ssl_close_notify(&m_ssl);
        m_stream->close();
        l.pushboolean(true);
        return {1};
    }

	void connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&connection::lnew);
		lua::bind::function(l,"configure",&connection::configure);
		lua::bind::function(l,"set_host",&connection::set_host);
		lua::bind::function(l,"handshake",&connection::handshake);
        lua::bind::function(l,"write",&connection::write);
        lua::bind::function(l,"read",&connection::read);
        lua::bind::function(l,"close",&connection::close);
	}
}
