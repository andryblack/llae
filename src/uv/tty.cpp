#include "tty.h"
#include "luv.h"
#include "loop.h"
#include "llae/app.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::tty,uv::stream)


namespace uv {

	tty::tty(loop& l,uv_file fd) {
		int r = uv_tty_init(l.native(),&m_tty,fd,0);
		UV_DIAG_CHECK(r);
		attach();
	}
	tty::~tty() {
	}

	lua::multiret tty::set_mode(lua::state& l,uv_tty_mode_t mode) {
		int res = uv_tty_set_mode(&m_tty,mode);
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	lua::multiret tty::reset_mode(lua::state& l) {
		int res = uv_tty_reset_mode();
		if (res < 0) {
			l.pushnil();
			uv::push_error(l,res);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	lua::multiret tty::lnew(lua::state& l,uv_file fd) {
		common::intrusive_ptr<tty> res{new tty(llae::app::get(l).loop(),fd)};
		lua::push(l,std::move(res));
		return {1};
	}

		
	void tty::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tty::lnew);
		lua::bind::function(l,"set_mode",&tty::set_mode);
		lua::bind::function(l,"reset_mode",&tty::reset_mode);

		l.pushinteger(UV_TTY_MODE_NORMAL);
		l.setfield(-2,"MODE_NORMAL");
		l.pushinteger(UV_TTY_MODE_RAW);
		l.setfield(-2,"MODE_RAW");
		l.pushinteger(UV_TTY_MODE_IO);
		l.setfield(-2,"MODE_IO");
		l.pushinteger(O_WRONLY);
		l.setfield(-2,"O_WRONLY");
		l.pushinteger(O_RDONLY);
		l.setfield(-2,"O_RDONLY");
		l.pushinteger(O_RDWR);
		l.setfield(-2,"O_RDWR");
		l.pushinteger(O_EXCL);
		l.setfield(-2,"O_EXCL");
		l.pushinteger(O_NOCTTY);
		l.setfield(-2,"O_NOCTTY");
		l.pushinteger(O_ASYNC);
		l.setfield(-2,"O_ASYNC");
		// l.pushinteger(O_DIRECT);
		// l.setfield(-2,"O_DIRECT");
		l.pushinteger(O_NONBLOCK);
		l.setfield(-2,"O_NONBLOCK");
	}
}
