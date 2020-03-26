#include "connection.h"
#include "llae/app.h"
#include "uv/luv.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <iostream>
#include <mbedtls/debug.h>

META_OBJECT_INFO(ssl::connection,meta::object)

namespace ssl {

	connection::connection( const uv::stream_ptr& stream ) : m_stream(stream) {
        mbedtls_debug_set_threshold(100);
		mbedtls_ssl_init( &m_ssl );
		mbedtls_ssl_config_init( &m_conf );
		m_write_req.data = this;
		m_write_buf = uv_buf_init(m_write_data_buf,CONN_BUFFER_SIZE);
	}

	connection::~connection() {
		mbedtls_ssl_free( &m_ssl );
		mbedtls_ssl_config_free( &m_conf );
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
			m_uv_error = status;
		}
		do_continue();
	}

	bool connection::do_handshake() {
		m_state = S_HANDSHAKE;
		int ret = mbedtls_ssl_handshake( &m_ssl );
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			m_ssl_error = ret;
			return false;
		}
		if (ret == 0) {
			m_state = S_CONNECTED;
		}
		return true;
	}

	void connection::finish_handshake() {
		auto& l = llae::app::get(m_stream->get_stream()->loop).lua();
		if (m_cont.valid()) {
			m_cont.push(l);
			auto toth = l.tothread(-1);
			l.pop(1);// thread
			int args;
			if (m_uv_error || m_ssl_error) {
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
			}
			m_cont.reset(l);
		}
		remove_ref();
	}

	void connection::do_continue() {
		if (m_state == S_HANDSHAKE) {

			if (m_uv_error || m_ssl_error || !do_handshake()) {
				finish_handshake();
			} else if (m_state == S_CONNECTED) {
				finish_handshake();
			} else {
				// continue handshake
				return;
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
	int connection::ssl_recv( const unsigned char *buf,
                                size_t len) {
		return -1;
	}

	int connection::lnew(lua_State* L) {
		lua::state l(L);
		auto stream = lua::stack<uv::stream_ptr>::get(l,2);
		common::intrusive_ptr<connection> conn{new connection(stream)};
		lua::push(l,std::move(conn));
		return 1;
	}

	void connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&connection::lnew);
		lua::bind::function(l,"configure",&connection::configure);
		lua::bind::function(l,"set_host",&connection::set_host);
		lua::bind::function(l,"handshake",&connection::handshake);
	}
}
