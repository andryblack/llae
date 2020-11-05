#include "os.h"
#include "luv.h"
#include "lua/bind.h"
#include <memory>

namespace uv {


	int os::homedir(lua_State* L) {
		lua::state l(L);
		size_t size = 1;
		char dummy;
		auto r = uv_os_homedir(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_homedir(data.get(),&size);
            if (r>=0) {
            	lua_pushlstring(L,data.get(),size);
				return 1;
			}
		}
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	int os::tmpdir(lua_State* L) {
		lua::state l(L);
        size_t size = 1;
        char dummy;
		auto r = uv_os_tmpdir(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_tmpdir(data.get(),&size);
            if (r>=0) {
            	lua_pushlstring(L,data.get(),size);
				return 1;
			}
		}
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	int os::getenv(lua_State* L) {
		lua::state l(L);
		auto name = l.checkstring(1);
        size_t size = 1;
        char dummy;
		auto r = uv_os_getenv(name,&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_getenv(name,data.get(),&size);
            if (r>=0) {
				lua_pushlstring(L,data.get(),size);
				return 1;
			}
		}
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	int os::setenv(lua_State* L) {
		lua::state l(L);
		auto name = l.checkstring(1);
		auto value = l.checkstring(2);
		auto r = uv_os_setenv(name,value);
		if (r<0) {
			l.pushnil();
			uv::push_error(l,r);
			return 2;
		}
		l.pushboolean(true);
		return 1;
	}
	int os::unsetenv(lua_State* L) {
		lua::state l(L);
		auto name = l.checkstring(1);
		auto r = uv_os_unsetenv(name);
		if (r<0) {
			l.pushnil();
			uv::push_error(l,r);
			return 2;
		}
		l.pushboolean(true);
		return 1;
	}
	int os::gethostname(lua_State* L) {
		lua::state l(L);
        size_t size = 1;
        char dummy;
		auto r = uv_os_gethostname(&dummy,&size);
		if (r == UV_ENOBUFS) {
            std::unique_ptr<char[]> data(new char[size]);
			r = uv_os_gethostname(data.get(),&size);
            if (r>=0) {
            	lua_pushlstring(L,data.get(),size);
				return 1;
			}
		}
		l.pushnil();
		uv::push_error(l,r);
		return 2;
	}
	int os::uname(lua_State* L) {
		lua::state l(L);
		uv_utsname_t n;
		auto r = uv_os_uname(&n);
		if (r<0) {
			l.pushnil();
			uv::push_error(l,r);
			return 2;
		}
		l.createtable(0,4);
		l.pushstring(n.sysname);
		l.setfield(-2,"sysname");
		l.pushstring(n.release);
		l.setfield(-2,"release");
		l.pushstring(n.version);
		l.setfield(-2,"version");
		l.pushstring(n.machine);
		l.setfield(-2,"machine");
		return 1;
	}

	void os::lbind(lua::state& l) {
		l.createtable();
		lua::bind::function(l,"homedir",&uv::os::homedir);
		lua::bind::function(l,"tmpdir",&uv::os::tmpdir);
		lua::bind::function(l,"getenv",&uv::os::getenv);
		lua::bind::function(l,"setenv",&uv::os::setenv);
		lua::bind::function(l,"unsetenv",&uv::os::unsetenv);
		lua::bind::function(l,"gethostname",&uv::os::gethostname);
		lua::bind::function(l,"uname",&uv::os::uname);
		l.setfield(-2,"os");
	}

}
