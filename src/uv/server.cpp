#include "server.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::server,uv::handle)


namespace uv {

	server::server() {
	}

	server::~server() {
	}

	void server::on_closed() {
        if (llae::app::closed(get_stream()->loop)) {
            m_conn_cb.release();
        } else {
            m_conn_cb.reset(llae::app::get(get_stream()->loop).lua());
        }
	}

	void server::connection_cb(uv_stream_t* s, int status) {
		if (s->data) {
			static_cast<server*>(s->data)->on_connection(status);
		}
	}

	void server::on_connection(int st) {
		if (m_conn_cb.valid()) {
			auto& l = llae::app::get(get_stream()->loop).lua();
			l.checkstack(2);
			m_conn_cb.push(l);

			if (st < 0) {
				uv::push_error(l,st);
			} else {
				l.pushnil();
			}
			auto r = l.pcall(1,0,0);
			if (r != lua::status::ok) {
				llae::app::show_error(l,r);
			}
		}
	}

	lua::multiret server::listen(lua::state& l) {
        if (m_conn_cb.valid()) {
            l.pushnil();
            l.pushstring("already listen");
            return {2};
        }
		auto backlog = l.checkinteger(2);
		l.checktype(3,lua::value_type::function);
		l.pushvalue(3);
		m_conn_cb.set(l);
		int res = uv_listen(get_stream(),int(backlog),&server::connection_cb);
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	lua::multiret server::accept(lua::state& l,const stream_ptr& stream) {
		int res = uv_accept(get_stream(),stream->get_stream());
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void server::stop(lua::state& l) {
		m_conn_cb.reset(l);
		close();
	}

	void server::lbind(lua::state& l) {
		lua::bind::function(l,"listen",&server::listen);
		lua::bind::function(l,"accept",&server::accept);
		lua::bind::function(l,"stop",&server::stop);
	}
}
