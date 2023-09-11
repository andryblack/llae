#ifndef __LLAE_UV_TCP_SERVER_H_INCLUDED__
#define __LLAE_UV_TCP_SERVER_H_INCLUDED__

#include "server.h"
#include "lua/state.h"
#include "stream.h"

namespace uv {

	class loop;
	class tcp_server : public server {
		META_OBJECT
	private:
		uv_tcp_t m_tcp;
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_tcp); }
		virtual uv_stream_t* get_stream() override final { return reinterpret_cast<uv_stream_t*>(&m_tcp); }
	protected:
		virtual ~tcp_server() override;
	public:
		explicit tcp_server(loop& l);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);

		lua::multiret bind(lua::state& l);
	};
	using tcp_server_ptr = common::intrusive_ptr<tcp_server>;

}

#endif /*__LLAE_UV_TCP_SERVER_H_INCLUDED__*/