#include "loop.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "fs.h"
#include "lua/bind.h"
#include "tcp_server.h"
#include "stream.h"
#include "tcp_connection.h"

namespace uv {
	

	static char uv_error_buf[1024];
    void error(lua::state& l,int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        l.error("UV error \"%q\"",err);
    }
    void push_error(lua::state& l,int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        l.pushstring(err);
    }
}

static int lua_uv_exepath(lua_State* L) {
	char path[PATH_MAX];
	lua::state l(L);
	size_t size = sizeof(path);
	auto r = uv_exepath(path, &size);
	if (r<0) {
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	l.pushstring(path);
	return 1;
}

int luaopen_uv(lua_State* L) {
	lua::state l(L);

	lua::bind::object<uv::handle>::register_metatable(l);
	lua::bind::object<uv::tcp_server>::register_metatable(l,&uv::tcp_server::lbind);
	lua::bind::object<uv::stream>::register_metatable(l,&uv::stream::lbind);
	lua::bind::object<uv::tcp_connection>::register_metatable(l,&uv::tcp_connection::lbind);

	l.createtable();
	lua::bind::object<uv::tcp_server>::get_metatable(l);
	l.setfield(-2,"tcp_server");
	lua::bind::object<uv::tcp_connection>::get_metatable(l);
	l.setfield(-2,"tcp_connection");
	lua::bind::function(l,"exepath",&lua_uv_exepath);
	uv::fs::lbind(l);
	return 1;
}