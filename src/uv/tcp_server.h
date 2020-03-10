#ifndef __LLAE_UV_TCP_SERVER_H_INCLUDED__
#define __LLAE_UV_TCP_SERVER_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "stream.h"

namespace uv {

	class tcp_server : public handle {
		META_OBJECT
	private:
		uv_tcp_t m_tcp;
		lua::ref m_conn_cb;
		static void connection_cb(uv_stream_t* server, int status);
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_tcp); }
		uv_stream_t* get_stream() { return reinterpret_cast<uv_stream_t*>(&m_tcp); }
	protected:
		explicit tcp_server(lua::state& l);
		virtual ~tcp_server() override;
		virtual void on_connection(int st);
		virtual void on_closed() override;
	public:
		static int lnew(lua_State* L);
		static void lbind(lua::state& l);

		lua::multiret bind(lua::state& l);
		lua::multiret listen(lua::state& l);
		void stop(lua::state& l);
		lua::multiret accept(lua::state& l,const stream_ptr& stream);
	};

}

#endif /*__LLAE_UV_TCP_SERVER_H_INCLUDED__*/