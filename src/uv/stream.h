#ifndef __LLAE_UV_STREAM_H_INCLUDED__
#define __LLAE_UV_STREAM_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "lua/ref.h"
#include "req.h"
#include <vector>

namespace uv {

	class write_req : public req {
		META_OBJECT
	private:
		uv_write_t	m_write;
		lua::ref m_cont;
		std::vector<uv_buf_t> m_bufs;
		std::vector<lua::ref> m_refs;
	protected:
		static void write_cb(uv_write_t* req, int status);
		void on_write(int status);
	public:
		write_req(lua::ref&& cont);
		~write_req();
		void put(lua::state& s);
		int write(uv_stream_t* s);
	};

	class stream : public handle {
		META_OBJECT
	private:
		lua::ref m_read_cont;
	protected:
		explicit stream();
		virtual ~stream() override;
	private:
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	protected:
		void on_read(ssize_t nread, const uv_buf_t* buf);
	public:
		virtual uv_stream_t* get_stream() = 0;
		static void lbind(lua::state& l);
		lua::multiret read(lua::state& l);
		lua::multiret write(lua::state& l);
	};
	typedef common::intrusive_ptr<stream> stream_ptr;

}

#endif /*__LLAE_UV_STREAM_H_INCLUDED__*/