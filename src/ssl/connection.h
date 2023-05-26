#ifndef __LLAE_SSL_CONNECTION_H_INCLUDED__
#define __LLAE_SSL_CONNECTION_H_INCLUDED__

#include "meta/object.h"
#include <mbedtls/ssl.h>
#include "uv/stream.h"
#include "ctx.h"
#include <deque>

namespace ssl {

	class connection : public uv::stream_read_consumer, public uv::readable_stream {
		META_OBJECT
	private:
        virtual void hold_ref() override final { add_ref(); }
        virtual void unhold_ref() override final { remove_ref(); }
        virtual lua::state& get_lua() override final;
        
		ctx_ptr m_ctx;
		mbedtls_ssl_context m_ssl;
		mbedtls_ssl_config m_conf;
        mbedtls_ssl_session m_session;
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
		int ssl_recv( unsigned char *buf,
                                size_t len);

		bool m_write_active = false;
		bool m_read_active = false;
		uv_write_t m_write_req;
		static const size_t CONN_BUFFER_SIZE = 1024 * 16;
		char m_write_data_buf[CONN_BUFFER_SIZE];
        uv::buffer_ptr m_read_data_buf;
		uv_buf_t m_write_buf;
		static void write_cb(uv_write_t* req, int status);
		void on_write_complete(int status);
		int m_uv_error = 0;
		int m_ssl_error = 0;
        bool is_error() const { return m_uv_error != 0 || m_ssl_error != 0;}
		enum {
			S_NONE,
			S_HANDSHAKE,
			S_CONNECTED,
            S_WRITE,
            S_UV_EOF,
            S_SHUTDOWN,
            S_SHUTDOWN_STREAM,
            S_CLOSED,
		} m_state = S_NONE;
		void do_continue();
        lua::ref m_write_cont;
		bool do_handshake();
        bool do_write();
        bool do_shutdown();
        bool do_shutdown_stream();
        
        void process_read();
        template <typename Handle>
        bool do_read(Handle handle);
		void finish_status(const char* state_point);
		void push_error(lua::state& l);
        enum {
            RS_NONE,
            RS_ACTIVE,
            RS_EOF,
            RS_ERROR
        } m_read_state = RS_NONE;
        
        struct readed_data {
            size_t size;
            size_t readded;
            uv::buffer_ptr data;
        };
        std::deque<readed_data> m_readed_data;
        class write_buffers : public uv::write_buffers {
            size_t m_front_writed = 0;
        public:
            const unsigned char* get_write_ptr() const {
                return reinterpret_cast<const unsigned char*>(get_buffers().front().base)+m_front_writed;
            }
            size_t get_write_size() const {
                return get_buffers().front().len - m_front_writed;
            }
            void consume(lua::state& l,size_t size) {
                m_front_writed += size;
                if (m_front_writed >= get_buffers().front().len) {
                    m_front_writed = 0;
                    pop_front(l);
                }
            }
            void reset(lua::state& l) {
                m_front_writed = 0;
                uv::write_buffers::reset(l);
            }
        } m_write_buffers;
        class shutdown_stream_req;
        void on_shutdown(int status);
        
        virtual bool on_read(uv::readable_stream* stream,ssize_t nread,
                             const uv::buffer_ptr&& buffer) override final;
        virtual void on_stream_closed(uv::readable_stream* s) override final;
        const char* m_active_op = nullptr;
        void begin_op(const char* op);
        void end_op(const char* op);
        common::intrusive_ptr<connection> m_active_op_lock;
        
	public:
		explicit connection( ctx_ptr&& ctx, uv::stream_ptr&& stream );
		~connection();
		static void lbind(lua::state& l);
		lua::multiret configure(lua::state& l);
		lua::multiret set_host(lua::state& l,const char* host);
		lua::multiret handshake(lua::state& l);
        lua::multiret write(lua::state& l);
        lua::multiret read(lua::state& l);
        lua::multiret close(lua::state& l);
        lua::multiret shutdown(lua::state& l);
        
        int start_read( const uv::stream_read_consumer_ptr& consumer ) override;
        void stop_read() override;
	};

}

#endif /*__LLAE_SSL_CONNECTION_H_INCLUDED__*/
