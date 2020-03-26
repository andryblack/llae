#ifndef __LLAE_SSL_CONNECTION_H_INCLUDED__
#define __LLAE_SSL_CONNECTION_H_INCLUDED__

#include "meta/object.h"
#include <mbedtls/ssl.h>
#include "uv/stream.h"

namespace ssl {

	class connection : public meta::object {
		META_OBJECT
	private:
		mbedtls_ssl_context m_ssl;
		mbedtls_ssl_config m_conf;
		static void dbg_cb(void *ctx, int level,
                      const char *file, int line,
                      const char *str);
		uv::stream_ptr m_stream;
		static int send_cb( void *ctx,
                                const unsigned char *buf,
                                size_t len );
		static int recv_cb( void *ctx,
                                unsigned char *buf,
                                size_t len );
		int ssl_send( const unsigned char *buf,
                                size_t len );
		int ssl_recv( const unsigned char *buf,
                                size_t len);

		bool m_write_active = false;
		bool m_read_active = false;
		uv_write_t m_write_req;
		static const size_t CONN_BUFFER_SIZE = 1024 * 16;
		char m_write_data_buf[CONN_BUFFER_SIZE];
		uv_buf_t m_write_buf;
		static void write_cb(uv_write_t* req, int status);
		void on_write_complete(int status);
		int m_uv_error = 0;
		int m_ssl_error = 0;
		enum {
			S_NONE,
			S_HANDSHAKE,
			S_CONNECTED,
		} m_state = S_NONE;
		void do_continue();
		lua::ref m_cont;
		bool do_handshake();
		void finish_handshake();
		void push_error(lua::state& l);
	public:
		explicit connection( const uv::stream_ptr& stream );
		~connection();
		static int lnew(lua_State* L);
		static void lbind(lua::state& l);
		lua::multiret configure(lua::state& l);
		lua::multiret set_host(lua::state& l,const char* host);
		lua::multiret handshake(lua::state& l);
	};

}

#endif /*__LLAE_SSL_CONNECTION_H_INCLUDED__*/