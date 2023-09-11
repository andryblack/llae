#include "pipe_server.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::pipe_server,uv::server)


namespace uv {

	pipe_server::pipe_server(loop& l,int ipc) {
		int r = uv_pipe_init(l.native(),&m_pipe,ipc);
        UV_DIAG_CHECK(r);
        attach();
	}

	pipe_server::~pipe_server() {
	}


	lua::multiret pipe_server::bind(lua::state& l) {
		const char* name = l.checkstring(2);
		int res = uv_pipe_bind(&m_pipe,name);
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}


	lua::multiret pipe_server::lnew(lua::state& l) {
		int ipc = l.optinteger(1,0);
		common::intrusive_ptr<pipe_server> server{new pipe_server(llae::app::get(l).loop(),ipc)};
		lua::push(l,std::move(server));
		return {1};
	}
	void pipe_server::lbind(lua::state& l) {
		lua::bind::function(l,"new",&pipe_server::lnew);
		lua::bind::function(l,"bind",&pipe_server::bind);
	}
}
