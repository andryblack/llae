#include "posix/fd.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "lua/raw_bind.h"
#include "lua/bind.h"
#include "posix/lposix.h"
#include <termios.h>

struct termios_mt  {
	static constexpr const char* name = "struct termios";
	using object = struct termios;
	static constexpr const auto fields = std::make_tuple(
		lua::field("c_iflag",&termios::c_iflag),
		lua::field("c_oflag",&termios::c_oflag),
		lua::field("c_cflag",&termios::c_cflag),
		lua::field("c_lflag",&termios::c_lflag),
		lua::field("c_cc",&termios::c_cc)
	);
};
using termios_stack = lua::stack<lua::check<lua::raw<termios_mt>>>;

static int lua_posix_tcsetattr(lua_State* L) {
	lua::state l(L);
	auto fd = lua::stack<posix::fd_ptr>::get(l,1);
	if (!fd) {
		l.argerror(1,"posix::fd");
	}
	int optional_actions = l.checkinteger(2);
	struct termios* tios = termios_stack::get(l,3);
	int r = tcsetattr(fd->get(),optional_actions,tios);
	if (r < 0) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	}
	l.pushinteger(r);
	return 1;
}

static int lua_posix_tcgetattr(lua_State* L) {
	lua::state l(L);
	auto fd = lua::stack<posix::fd_ptr>::get(l,1);
	if (!fd) {
		l.argerror(1,"posix::fd");
	}
	struct termios tios = {};
	int r = tcgetattr(fd->get(),&tios);
	if (r < 0) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	}
	termios_stack::push(l,tios);
	return 1;
}

static int lua_posix_cfsetispeed(lua_State* L) {
	lua::state l(L);
	auto ios = termios_stack::get(l,1);
	int r = cfsetispeed(ios,l.checkinteger(2));
	if (r < 0) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	}
	l.pushboolean(true);
	return 1;
}

static int lua_posix_cfsetospeed(lua_State* L) {
	lua::state l(L);
	auto ios = termios_stack::get(l,1);
	int r = cfsetospeed(ios,l.checkinteger(2));
	if (r < 0) {
		l.pushnil();
		posix::push_error_errno(l);
		return 2;
	}
	l.pushboolean(true);
	return 1;
}

int luaopen_posix_termios(lua_State* L) {
	lua::state l(L);
	lua::register_raw_metatable<termios_mt>(l);
	
	l.createtable();

	lua::bind::function(l,"tcsetattr",&lua_posix_tcsetattr);
	lua::bind::function(l,"tcgetattr",&lua_posix_tcgetattr);

	lua::bind::function(l,"cfsetispeed",&lua_posix_cfsetispeed);
	lua::bind::function(l,"cfsetospeed",&lua_posix_cfsetospeed);

	BIND_M(TCSANOW)
	BIND_M(TCSADRAIN)
	BIND_M(TCSAFLUSH)

	BIND_M(IGNBRK)
	BIND_M(BRKINT)
	BIND_M(IGNPAR)
	BIND_M(PARMRK)
	BIND_M(INPCK)
	BIND_M(ISTRIP)
	BIND_M(INLCR)
	BIND_M(IGNCR)
	BIND_M(ICRNL)
#ifdef IUCLC
	BIND_M(IUCLC)
#endif
	BIND_M(IXON)
	BIND_M(IXANY)
	BIND_M(IXOFF)
	BIND_M(IMAXBEL)

	BIND_M(OPOST)

#ifdef OLCUC
	BIND_M(OLCUC)
#endif
	BIND_M(ONLCR)
	BIND_M(OCRNL)
	BIND_M(ONOCR)
	BIND_M(ONLRET)
	BIND_M(OFILL)
	BIND_M(OFDEL)
	BIND_M(NLDLY)
	BIND_M(CRDLY)
	BIND_M(TABDLY)
	BIND_M(BSDLY)
	BIND_M(VTDLY)
	BIND_M(FFDLY)

#ifdef CBAUD
	BIND_M(CBAUD)
#endif
#ifdef CBAUDEX
	BIND_M(CBAUDEX)
#endif

	BIND_M(CSIZE)
	BIND_M(CSTOPB)
	BIND_M(CREAD)
	BIND_M(PARENB)
	BIND_M(PARODD)
	BIND_M(HUPCL)
	BIND_M(CLOCAL)
#ifdef LOBLK
	BIND_M(LOBLK)
#endif
#ifdef CIBAUD
	BIND_M(CIBAUD)
#endif
	BIND_M(CRTSCTS)

	BIND_M(ISIG)
	BIND_M(ICANON)
#ifdef XCASE
	BIND_M(XCASE)
#endif
	BIND_M(ECHO)
	BIND_M(ECHOE)
	BIND_M(ECHOK)
	BIND_M(ECHONL)
	BIND_M(ECHOCTL)
	BIND_M(ECHOPRT)
	BIND_M(ECHOKE)
#ifdef DEFECHO
	BIND_M(DEFECHO)
#endif
	BIND_M(FLUSHO)
	BIND_M(NOFLSH)
	BIND_M(TOSTOP)
	BIND_M(PENDIN)
	BIND_M(IEXTEN)

	BIND_M(VINTR)
	BIND_M(VQUIT)
	BIND_M(VERASE)
	BIND_M(VKILL)
	BIND_M(VEOF)
	BIND_M(VMIN)
	BIND_M(VEOL)
	BIND_M(VTIME)
	BIND_M(VEOL2)
#ifdef VSWTCH
	BIND_M(VSWTCH)
#endif
	BIND_M(VSTART)
	BIND_M(VSTOP)
	BIND_M(VSUSP)
	BIND_M(VDSUSP)
	BIND_M(VLNEXT)
	BIND_M(VWERASE)
	BIND_M(VREPRINT)
	BIND_M(VDISCARD)
	BIND_M(VSTATUS)

	BIND_M(B9600)
	BIND_M(B19200)
	BIND_M(B38400)
	BIND_M(B57600)
	BIND_M(B115200)
	BIND_M(B230400)

	BIND_M(CS8)

	return 1;
}