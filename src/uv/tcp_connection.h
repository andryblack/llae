#ifndef __LLAE_UV_TCP_CONNECTION_H_INCLUDED__
#define __LLAE_UV_TCP_CONNECTION_H_INCLUDED__

#include "stream.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {

	class loop;
	
	class tcp_connection : public stream {
		META_OBJECT
	private:
		uv_tcp_t m_tcp;
		class connect_req;
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_tcp); }
		virtual uv_stream_t* get_stream() override final { return reinterpret_cast<uv_stream_t*>(&m_tcp); }
	protected:
		virtual ~tcp_connection() override;
		uv_tcp_t* get_tcp() { return &m_tcp; }
	public:
		explicit tcp_connection(uv::loop& loop);
		static lua::multiret lnew(lua::state& l);
		lua::multiret connect(lua::state& l);
        lua::multiret getpeername(lua::state& l);
        lua::multiret keepalive(lua::state& l);
        lua::multiret nodelay(lua::state& l);
		static void lbind(lua::state& l);
	};
	typedef common::intrusive_ptr<tcp_connection> tcp_connection_ptr;

}

#endif /*__LLAE_UV_TCP_CONNECTION_H_INCLUDED__*/
