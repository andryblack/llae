#include "loop.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/bind.h"
#include "tcp_server.h"
#include "stream.h"
#include "tcp_connection.h"
#include "dns.h"
#include "fs.h"
#include "os.h"
#include "udp.h"
#include <iostream>
#include <memory>

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
    void print_error(int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        std::cout << "ERR: " << err << std::endl;
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

static int lua_uv_cwd(lua_State* L) {
	lua::state l(L);
	size_t size = 1;
	char dummy;
	auto r = uv_cwd(&dummy,&size);
	if (r == UV_ENOBUFS) {
        std::unique_ptr<char[]> data(new char[size]);
		r = uv_cwd(data.get(),&size);
        if (r>=0) {
			lua_pushlstring(L,data.get(),size);
			return 1;
		}
	}
	l.pushnil();
	uv::push_error(l,r);
	return 2;
}

static int lua_uv_chdir(lua_State* L) {
	lua::state l(L);
	const char* dir = l.checkstring(1);
	auto r = uv_chdir(dir);
	if (r>0) {
		l.pushboolean(true);
		return 1;
	}
	l.pushnil();
	uv::push_error(l,r);
	return 2;
}

int luaopen_uv(lua_State* L) {
	lua::state l(L);

	lua::bind::object<uv::handle>::register_metatable(l);
	lua::bind::object<uv::tcp_server>::register_metatable(l,&uv::tcp_server::lbind);
	lua::bind::object<uv::stream>::register_metatable(l,&uv::stream::lbind);
	lua::bind::object<uv::tcp_connection>::register_metatable(l,&uv::tcp_connection::lbind);
	lua::bind::object<uv::buffer>::register_metatable(l,&uv::buffer::lbind);
    lua::bind::object<uv::udp>::register_metatable(l,&uv::udp::lbind);

	l.createtable();
	lua::bind::object<uv::buffer>::get_metatable(l);
	l.setfield(-2,"buffer");
	lua::bind::object<uv::tcp_server>::get_metatable(l);
	l.setfield(-2,"tcp_server");
	lua::bind::object<uv::tcp_connection>::get_metatable(l);
	l.setfield(-2,"tcp_connection");
    lua::bind::object<uv::udp>::get_metatable(l);
    l.setfield(-2,"udp");
	lua::bind::function(l,"exepath",&lua_uv_exepath);
	lua::bind::function(l,"getaddrinfo",&uv::getaddrinfo_req::getaddrinfo);
	lua::bind::function(l,"cwd",&lua_uv_cwd);
	lua::bind::function(l,"chdir",&lua_uv_chdir);
	uv::fs::lbind(l);
	uv::os::lbind(l);
	return 1;
}
