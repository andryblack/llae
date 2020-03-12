#ifndef __LLAE_UV_STREAM_H_INCLUDED__
#define __LLAE_UV_STREAM_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "lua/ref.h"
#include "req.h"
#include <vector>

namespace uv {

	class stream;
	typedef common::intrusive_ptr<stream> stream_ptr;


	class write_req : public req {
	private:
		uv_write_t	m_write;
		stream_ptr  m_stream;
		lua::ref m_cont;
		std::vector<uv_buf_t> m_bufs;
		std::vector<lua::ref> m_refs;
	protected:
		static void write_cb(uv_write_t* req, int status);
		void on_write(int status);
	public:
		write_req(stream_ptr&& stream,lua::ref&& cont);
		~write_req();
		void put(lua::state& s);
		int write();
	};

	class shutdown_req : public req {
	private:
		uv_shutdown_t m_shutdown;
		stream_ptr  m_stream;
		lua::ref m_cont;
	protected:
		static void shutdown_cb(uv_shutdown_t* req, int status);
		void on_shutdown(int status);
	public:
		shutdown_req(stream_ptr&& stream,lua::ref&& cont);
		~shutdown_req();
		int shutdown();
	};

	class stream : public handle {
		META_OBJECT
	private:
		lua::ref m_read_cont;
	protected:
		explicit stream();
		virtual ~stream() override;
		virtual void on_closed() override;
	private:
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	protected:
		bool on_read(ssize_t nread, const uv_buf_t* buf);
	public:
		virtual uv_stream_t* get_stream() = 0;
		static void lbind(lua::state& l);
		lua::multiret read(lua::state& l);
		lua::multiret write(lua::state& l);
		lua::multiret shutdown(lua::state& l);
		lua::multiret send(lua::state& l);
		void close();
	};

}

#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/