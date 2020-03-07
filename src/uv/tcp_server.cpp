#include "tcp_server.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::tcp_server,uv::handle)


namespace uv {

	tcp_server::tcp_server(lua::state& l) {
		int r = uv_tcp_init(llae::app::get(l).loop().native(),&m_tcp);
		check_error(l,r);
		attach();
	}
	tcp_server::~tcp_server() {

	}

	lua::multiret tcp_server::bind(lua::state& l) {
		const char* host = l.checkstring(2);
		int port = l.checkinteger(3);
		struct sockaddr_storage addr;
		if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
	      	uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
	    	l.error("invalid IP address or port [%s:%d]", host, port);
	   	}
		int res = uv_tcp_bind(&m_tcp,(struct sockaddr*)&addr,0);
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void tcp_server::connection_cb(uv_stream_t* server, int status) {
		if (server->data) {
			static_cast<tcp_server*>(server->data)->on_connection(status);
		}
	}

	void tcp_server::on_connection(int st) {
		if (m_conn_cb.valid()) {
			auto& l = llae::app::get(m_tcp.loop).lua();
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

	lua::multiret tcp_server::listen(lua::state& l) {
		int backlog = l.checkinteger(2);
		l.checktype(3,lua::value_type::function);
		l.pushvalue(3);
		m_conn_cb.set(l);
		int res = uv_listen(get_stream(),backlog,&tcp_server::connection_cb);
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	lua::multiret tcp_server::accept(lua::state& l,const stream_ptr& stream) {
		int res = uv_accept(get_stream(),stream->get_stream());
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	void tcp_server::stop(lua::state& l) {
		m_conn_cb.reset(l);
		close();
	}

	int tcp_server::lnew(lua_State* L) {
		lua::state l(L);
		common::intrusive_ptr<tcp_server> server{new tcp_server(l)};
		lua::push(l,std::move(server));
		return 1;
	}
	void tcp_server::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_server::lnew);
		lua::bind::function(l,"bind",&tcp_server::bind);
		lua::bind::function(l,"listen",&tcp_server::listen);
		lua::bind::function(l,"accept",&tcp_server::accept);
		lua::bind::function(l,"stop",&tcp_server::stop);
	}
}