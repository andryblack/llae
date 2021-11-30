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
#include "tty.h"
#include "poll.h"
#include "process.h"
#include "timer.h"
#include "pipe.h"
#include <iostream>
#include <memory>

namespace uv {
	

	static char uv_error_buf[1024];
    void error(lua::state& l,int e) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        l.error("UV error \"%q\"",err);
    }
    void error(int e,const char* file,int line) {
        const char* err = uv_strerror_r(e, uv_error_buf, sizeof(uv_error_buf));
        if (!err) err = "unknown";
        llae::report_diag_error(err,file,line);
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
    std::string get_error(int r) {
    	char uv_error_buf[1024];
    	const char* err = uv_strerror_r(r, uv_error_buf, sizeof(uv_error_buf));
    	if (err) return err;
    	return "unknown";
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

std::string uv::get_cwd() {
    size_t size = 1;
    char dummy;
    auto r = uv_cwd(&dummy,&size);
    if (r == UV_ENOBUFS) {
        std::unique_ptr<char[]> data(new char[size]);
        r = uv_cwd(data.get(),&size);
        if (r>=0) {
            return data.get();
        }
    }
    return "";
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
    lua::bind::object<uv::tty>::register_metatable(l,&uv::tty::lbind);
    lua::bind::object<uv::poll>::register_metatable(l,&uv::poll::lbind);
    lua::bind::object<uv::process>::register_metatable(l,&uv::process::lbind);
    lua::bind::object<uv::pipe>::register_metatable(l,&uv::pipe::lbind);

	l.createtable();
	lua::bind::object<uv::buffer>::get_metatable(l);
	l.setfield(-2,"buffer");
	lua::bind::object<uv::tcp_server>::get_metatable(l);
	l.setfield(-2,"tcp_server");
	lua::bind::object<uv::tcp_connection>::get_metatable(l);
	l.setfield(-2,"tcp_connection");
    lua::bind::object<uv::udp>::get_metatable(l);
    l.setfield(-2,"udp");
    lua::bind::object<uv::tty>::get_metatable(l);
    l.setfield(-2,"tty");
    lua::bind::object<uv::poll>::get_metatable(l);
    l.setfield(-2,"poll");
    lua::bind::object<uv::process>::get_metatable(l);
    l.setfield(-2,"process");
    lua::bind::object<uv::pipe>::get_metatable(l);
    l.setfield(-2,"pipe");
    
	lua::bind::function(l,"exepath",&lua_uv_exepath);
	lua::bind::function(l,"getaddrinfo",&uv::getaddrinfo_req::getaddrinfo);
	lua::bind::function(l,"cwd",&lua_uv_cwd);
	lua::bind::function(l,"chdir",&lua_uv_chdir);
	lua::bind::function(l,"pause",&uv::timer_pause::pause);
	uv::fs::lbind(l);
	uv::os::lbind(l);
	return 1;
}
