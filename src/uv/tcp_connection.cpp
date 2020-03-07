#include "tcp_connection.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::tcp_connection,uv::stream)


namespace uv {

	tcp_connection::tcp_connection(lua::state& l) {
		int r = uv_tcp_init(llae::app::get(l).loop().native(),&m_tcp);
		check_error(l,r);
		attach();
	}
	tcp_connection::~tcp_connection() {

	}

	int tcp_connection::lnew(lua_State* L) {
		lua::state l(L);
		common::intrusive_ptr<tcp_connection> connection{new tcp_connection(l)};
		lua::push(l,std::move(connection));
		return 1;
	}
	void tcp_connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_connection::lnew);
	}
}