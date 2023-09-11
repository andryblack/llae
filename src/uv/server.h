#pragma once

#include "handle.h"
#include "stream.h"
#include "lua/ref.h"

namespace uv {

	class server : public handle {
		META_OBJECT
	private:
		lua::ref m_conn_cb;
		static void connection_cb(uv_stream_t* server, int status);
	protected:
		virtual uv_stream_t* get_stream() = 0;
		virtual void on_connection(int st);
		virtual void on_closed() override;
		explicit server();
		~server();
	public:
		static void lbind(lua::state& l);

		lua::multiret listen(lua::state& l);
		void stop(lua::state& l);
		lua::multiret accept(lua::state& l,const stream_ptr& stream);
	};
}