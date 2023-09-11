#include "tcp_server.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::tcp_server,uv::server)


namespace uv {

	tcp_server::tcp_server(loop& l) {
		int r = uv_tcp_init(l.native(),&m_tcp);
		UV_DIAG_CHECK(r);
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


	lua::multiret tcp_server::lnew(lua::state& l) {
		common::intrusive_ptr<tcp_server> server{new tcp_server(llae::app::get(l).loop())};
		lua::push(l,std::move(server));
		return {1};
	}
	void tcp_server::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_server::lnew);
		lua::bind::function(l,"bind",&tcp_server::bind);
	}
}
