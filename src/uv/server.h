#pragma once

#include "handle.h"
#include "stream.h"
#include "llae/promise.h"

namespace uv {

	class server : public handle {
		META_OBJECT
	private:
		static void connection_cb(uv_stream_t* server, int status);
		llae::result_promise_ptr<void> m_listen_promise;
		enum class state_t {
			s_none,
			s_listening,
			s_closing
		} m_state = state_t::s_none;
	protected:
		virtual uv_stream_t* get_stream() = 0;
		virtual void on_connection(int st);
		virtual void on_closed() override;
		explicit server();
		~server();
	public:
		static void lbind(lua::state& l);

		llae::result_promise_ptr<void> listen(lua::state& l);
		void stop(lua::state& l);
		llae::result<void> accept(lua::state& l,const stream_ptr& stream);
	};
}