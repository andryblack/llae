#include "posix/fd.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "lua/raw_bind.h"
#include "lua/bind.h"
#include "posix/lposix.h"

#ifndef _WIN32
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


// static int lua_posix_setbaudrate(lua_State* L) {
// 	lua::state l(L);
// 	auto fd = lua::stack<posix::fd_ptr>::get(l,1);
// 	if (!fd) {
// 		l.argerror(1,"posix::fd");
// 	}
// 	auto baudrate = l.checkinteger(2);
// 	auto ios = termios_stack::get(l,3);
// 	struct termios * termios = ios;
// 	struct termios s_termios;
// 	if (!termios) {
// 		termios = &s_termios;
// 		int r = tcgetattr(fd->get(),&tios);
// 		if (r < 0) {
// 			l.pushnil();
// 			posix::push_error_errno(l);
// 			return 2;
// 		}
// 	}

// #ifndef __APPLE__

//     speed_t brate = baudrate; // let you override switch below if needed
//     switch(baudrate) {
//         case    9600: brate=B9600;    break;
//         case   19200: brate=B19200;   break;
//         case   38400: brate=B38400;   break;
//         case 57600:  brate=B57600;  break;
//         case 115200: brate=B115200; break;
// #ifdef B230400
//         case 230400: brate=B230400; break;
// #endif
// #ifdef B460800
//         case 460800: brate=B460800; break;
// #endif
// #ifdef B500000
//         case  500000: brate=B500000;  break;
// #endif
// #ifdef B576000
//         case  576000: brate=B576000;  break;
// #endif
// #ifdef B921600
//         case 921600: brate=B921600; break;
// #endif
// #ifdef B1000000
//         case 1000000: brate=B1000000; break;
// #endif
// #ifdef B1152000
//         case 1152000: brate=B1152000; break;
// #endif
// #ifdef B1500000
//         case 1500000: brate=B1500000; break;
// #endif
// #ifdef B2000000
//         case 2000000: brate=B2000000; break;
// #endif
// #ifdef B2500000
//         case 2500000: brate=B2500000; break;
// #endif
// #ifdef B3000000
//         case 3000000: brate=B3000000; break;
// #endif
// #ifdef B3500000
//         case 3500000: brate=B3500000; break;
// #endif
// #ifdef B400000
//         case 4000000: brate=B4000000; break;
// #endif
//         default:
//             log_error("can't set baudrate %dn", baudrate );
//             return -1;
//     }
//     cfsetospeed(termios, brate);
//     cfsetispeed(termios, brate);
// #endif

//     // also set options for __APPLE__ to enforce write drain
//     // Mac OS Mojave: tcsdrain did not work as expected

//     if (ios) {
// 	    if( tcsetattr(fd->get(), TCSADRAIN, termios) < 0) {
// 	    	l.pushnil();
// 	    	l.pushstring("Couldn't set term attributes");
// 	        return 2;
// 	    }
// 	}

// #ifdef __APPLE__
//     // From https://developer.apple.com/library/content/samplecode/SerialPortSample/Listings/SerialPortSample_SerialPortSample_c.html

//     // The IOSSIOSPEED ioctl can be used to set arbitrary baud rates
//     // other than those specified by POSIX. The driver for the underlying serial hardware
//     // ultimately determines which baud rates can be used. This ioctl sets both the input
//     // and output speed.

//     speed_t speed = baudrate;
//     if (ioctl(fd->get(), IOSSIOSPEED, &speed) == -1) {
//     	l.pushnil();
//         l.pushfstring("error calling ioctl(..., IOSSIOSPEED, %u) - %s(%d).\n", baudrate, strerror(errno), errno);
//         return 2;
//     }
// #endif
//     l.pushboolean(true);
//     return 1;
// }

int luaopen_posix_termios(lua_State* L) {
	lua::state l(L);
	lua::register_raw_metatable<termios_mt>(l);
	
	l.createtable();

	lua::bind::function(l,"tcsetattr",&lua_posix_tcsetattr);
	lua::bind::function(l,"tcgetattr",&lua_posix_tcgetattr);

	lua::bind::function(l,"cfsetispeed",&lua_posix_cfsetispeed);
	lua::bind::function(l,"cfsetospeed",&lua_posix_cfsetospeed);

	//lua::bind::function(l,"setbaudrate",&lua_posix_setbaudrate);
	
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
#ifdef VDSUSP
	BIND_M(VDSUSP)
#endif
	BIND_M(VLNEXT)
	BIND_M(VWERASE)
	BIND_M(VREPRINT)
	BIND_M(VDISCARD)
#ifdef VSTATUS
	BIND_M(VSTATUS)
#endif

	BIND_M(B9600)
	BIND_M(B19200)
	BIND_M(B38400)
	BIND_M(B57600)
	BIND_M(B115200)
	BIND_M(B230400)
#ifdef B460800
	BIND_M(B460800)
#endif
#ifdef B500000
    BIND_M(B500000)
#endif
#ifdef B576000
    BIND_M(B576000)
#endif
#ifdef B921600
    BIND_M(B921600)
#endif
#ifdef B1000000
    BIND_M(B1000000)
#endif
#ifdef B1152000
    BIND_M(B1152000)
#endif
#ifdef B1500000
    BIND_M(B1500000)
#endif
#ifdef B2000000
    BIND_M(B2000000)
#endif
#ifdef B2500000
    BIND_M(B2500000)
#endif
#ifdef B3000000
    BIND_M(B3000000)
#endif
#ifdef B3500000
    BIND_M(B3500000)
#endif
#ifdef B400000
    BIND_M(B400000)
#endif

	BIND_M(CS8)

	return 1;
}
#else
int luaopen_posix_termios(lua_State* L) {
	return 0;
}
#endif