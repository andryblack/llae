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

	tty::tty(loop& l,posix::fd_ptr && fd) : m_fd(std::move(fd)) {
		int r = uv_tty_init(l.native(),&m_tty,m_fd->get(),0);
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

	lua::multiret tty::lnew(lua::state& l) {
		posix::fd_ptr fdp = lua::stack<posix::fd_ptr>::get(l,1);
		if (!fdp) {
			uv_file fd = l.checkinteger(1);
			common::intrusive_ptr<tty> res{new tty(llae::app::get(l).loop(),fd)};
			lua::push(l,std::move(res));
		} else {
			common::intrusive_ptr<tty> res{new tty(llae::app::get(l).loop(),std::move(fdp))};
			lua::push(l,std::move(res));
		}
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
		
	}
}
