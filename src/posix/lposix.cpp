#include "lposix.h"
#include "fd.h"
#include "lua/bind.h"
#include "lua/stack.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

namespace posix {

	void push_error_errno(lua::state& l) {
		auto es = ::strerror(errno);
		l.pushstring(es);
	}	

}


static int lua_posix_open(lua_State* L) {
	lua::state l(L);
	const char* pathname = l.checkstring(1);
	int flags = l.checkinteger(2);
	int fd = open(pathname,flags);
	if (fd == -1) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	} else {
		lua::push(l,posix::fd_ptr(new posix::fd(fd)));
	}
	return 1;
}

static int lua_posix_dup2(lua_State* L) {
	lua::state l(L);
	auto fd = lua::stack<posix::fd_ptr>::get(l,1);
	auto from = l.checkinteger(2);
	if (!fd) {
		l.argerror(1,"need fd");
	}
	int res = dup2(fd->get(),from);
	if (res == -1) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	}
	l.pushinteger(res);
	return 1;
}


int luaopen_posix(lua_State* L) {
	lua::state l(L);

	lua::bind::object<posix::fd>::register_metatable(l,&posix::fd::lbind);

	l.createtable();

	lua::bind::function(l,"open",&lua_posix_open);
	lua::bind::function(l,"dup2",&lua_posix_dup2);

	BIND_M(O_WRONLY);
	BIND_M(O_RDONLY);
	BIND_M(O_RDWR);
	BIND_M(O_EXCL);
#ifdef O_NOCTTY
	BIND_M(O_NOCTTY);
#endif
#ifdef O_ASYNC
	BIND_M(O_ASYNC);
#endif
#ifdef O_DIRECT
	BIND_M(O_DIRECT);
#endif
#ifdef O_NONBLOCK
	BIND_M(O_NONBLOCK);
#endif

#ifdef F_GETFD
	BIND_M(F_GETFD);
#endif
#ifdef F_GETFL
	BIND_M(F_GETFL);
#endif
#ifdef F_GETOWN
	BIND_M(F_GETOWN);
#endif
#ifdef F_GETSIG
	BIND_M(F_GETSIG);
#endif
#ifdef F_GETLEASE
	BIND_M(F_GETLEASE);
#endif
#ifdef F_SETFD
	BIND_M(F_SETFD);
#endif
#ifdef F_SETFL
	BIND_M(F_SETFL);
#endif
#ifdef F_SETOWN
	BIND_M(F_SETOWN);
#endif
#ifdef F_SETSIG
	BIND_M(F_SETSIG);
#endif
#ifdef F_SETLEASE
	BIND_M(F_SETLEASE);
#endif
#ifdef F_NOTIFY
	BIND_M(F_NOTIFY);
#endif
#ifdef STDERR_FILENO
	BIND_M(STDERR_FILENO);
#endif
#ifdef STDOUT_FILENO
	BIND_M(STDOUT_FILENO);
#endif
#ifdef STDIN_FILENO
	BIND_M(STDIN_FILENO);
#endif

	return 1;
}
