#pragma once

#include "uv/stream.h"

namespace db {

	class resp3 : public uv::stream_read_consumer {
		META_OBJECT
	private:
		uv::stream_ptr m_stream;
		lua::ref m_cont;
		class write_buffer_req;
		class write_lua_req;
	protected:
		static uv::buffer_ptr encode(lua::state& l,int start_idx);
		void on_write_complete(lua::state& l,int status);
		const uv::stream_ptr& get_stream() const { return m_stream; }

		virtual bool on_read(uv::readable_stream* s,ssize_t nread, uv::buffer_ptr& buffer) override;
	public:
		explicit resp3(  uv::stream_ptr&& stream );
		lua::multiret cmd(lua::state& l);
		lua::multiret pipelining(lua::state& l);
		lua::multiret close(lua::state& l);
		static lua::multiret gen_req(lua::state& l);
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};

	using resp3_ptr = common::intrusive_ptr<resp3>;
}