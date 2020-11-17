#ifndef __LLAE_UV_STREAM_H_INCLUDED__
#define __LLAE_UV_STREAM_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "lua/ref.h"
#include "req.h"
#include "buffer.h"
#include <vector>

namespace uv {

	class stream;
	typedef common::intrusive_ptr<stream> stream_ptr;

	class write_req : public req {
	private:
		uv_write_t	m_write;
		stream_ptr  m_stream;
	protected:
		uv_loop_t* get_loop() { return m_write.handle->loop; }
		static void write_cb(uv_write_t* req, int status);
		virtual void on_write(int status) = 0;
		int start_write(const uv_buf_t* buffers,size_t count);
	public:
		write_req(stream_ptr&& stream);
		~write_req();
	};
	using write_req_ptr = common::intrusive_ptr<write_req>;

	class write_lua_req : public write_req {
	private:
		lua::ref m_cont;
        write_buffers m_buffers;
    protected:
    	virtual void on_write(int status) override;
    public:
    	write_lua_req(stream_ptr&& stream,lua::ref&& cont);
    	bool put(lua::state& s);
        bool empty() const { return m_buffers.empty(); }
        void reset(lua::state& s);
		int write();
	};

	class write_buffer_req : public write_req {
	private:
		uv::buffer_ptr m_buffer;
	protected:
		virtual void on_write(int status) override;
	public:
		explicit write_buffer_req(stream_ptr&& stream,uv::buffer_ptr&& s);
		~write_buffer_req();
		int write();
	};

	class shutdown_req : public req {
	private:
		uv_shutdown_t m_shutdown;
		stream_ptr  m_stream;
	protected:
		uv_loop_t* get_loop() { return m_shutdown.handle->loop; }
		static void shutdown_cb(uv_shutdown_t* req, int status);
		virtual void on_shutdown(int status) = 0;
	public:
		shutdown_req(stream_ptr&& stream);
		~shutdown_req();
		int shutdown();
	};

	class shutdown_lua_req : public shutdown_req {
	private:
		lua::ref m_cont;
	protected:
		virtual void on_shutdown(int status) override;
	public:
		shutdown_lua_req(stream_ptr&& stream,lua::ref&& cont);
	};

   
    class stream_read_consumer : public meta::object {
    public:
        virtual bool on_read(stream* s,ssize_t nread, const buffer_ptr&& buffer) = 0;
        virtual void on_stream_closed(stream* s) {}
    };
    typedef common::intrusive_ptr<stream_read_consumer> stream_read_consumer_ptr;

	class stream : public handle {
		META_OBJECT
	private:
        stream_read_consumer_ptr m_read_consumer;
		
	protected:
		explicit stream();
		virtual ~stream() override;
		virtual void on_closed() override;
	private:
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	protected:
		bool on_read(ssize_t nread, const buffer_ptr&& buffer);
	public:
		virtual uv_stream_t* get_stream() = 0;
		static void lbind(lua::state& l);
		bool write(buffer_ptr&& buf);
		lua::multiret read(lua::state& l);
		lua::multiret write(lua::state& l);
		lua::multiret shutdown(lua::state& l);
		lua::multiret send(lua::state& l);
        int start_read( const stream_read_consumer_ptr& consumer );
        void stop_read();
		void close();
	};

}

#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/
