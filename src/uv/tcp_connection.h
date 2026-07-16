#ifndef __LLAE_UV_TCP_CONNECTION_H_INCLUDED__
#define __LLAE_UV_TCP_CONNECTION_H_INCLUDED__

#include "stream.h"
#include "lua/state.h"
#include "llae/promise.h"
#include <string_view>

namespace llae {
	class loop;
}

namespace uv {

	class loop;
	
	/// @luabind
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
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		/// @luabind(async=true)
		llae::result_promise_ptr<void> connect(llae::loop& l, std::string_view host, int port);
        /// @luabind
        lua::multiret getpeername(lua::state& l);
        /// @luabind
        llae::result<> keepalive(std::optional<int> delay);
        /// @luabind
        llae::result<> nodelay(bool enable);
	};
	typedef common::intrusive_ptr<tcp_connection> tcp_connection_ptr;

}

#endif /*__LLAE_UV_TCP_CONNECTION_H_INCLUDED__*/
