#include "fd.h"
#include "lposix.h"
#include "lua/bind.h"
#include "uv/buffer.h"
#include <fcntl.h>
#include <unistd.h>

META_OBJECT_INFO(posix::fd,meta::object)

namespace posix {

	fd::~fd() {
		if (m_fd != -1) {
			::close(m_fd);
		}
	}

	lua::multiret fd::close(lua::state& l) {
		int r = ::close(m_fd);
		if (r == -1) {
			l.pushnil();
			push_error_errno(l);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}

	lua::multiret fd::read(lua::state& l) {
		uv::buffer_ptr buffer = lua::stack<uv::buffer_ptr>::get(l,2);
		if (!buffer) {
			buffer = uv::buffer::alloc(l.checkinteger(2));
		}
		auto readed = ::read(m_fd,buffer->get_base(),buffer->get_capacity());
		if (readed < 0) {
			l.pushnil();
			push_error_errno(l);
			return {2};
		}
		buffer->set_len(readed);
		lua::push(l,buffer);
		return {1};
	}
	lua::multiret fd::write(lua::state& l) {
		uv::buffer_ptr buffer = lua::stack<uv::buffer_ptr>::get(l,2);
		const void* data = nullptr;
		size_t len = 0;
		if (!buffer) {
			data = l.checklstring(2,len);
		} else {
			data = buffer->get_base();
			len = buffer->get_len();
		}

		auto writed = ::write(m_fd,data,len);
		if (writed < 0) {
			l.pushnil();
			push_error_errno(l);
			return {2};
		}
		l.pushinteger(writed);
		return {1};
	}

	lua::multiret fd::fcntl(lua::state& l) {
		int cmd =  l.checkinteger(2);
		int res;
		switch (cmd) {
			case F_GETFD:
			case F_GETFL:
			case F_GETOWN:
#ifdef F_GETSIG
			case F_GETSIG:
#endif
#ifdef F_GETLEASE
			case F_GETLEASE:
#endif
				res = ::fcntl(m_fd,cmd);
				break;
			case F_SETFD:
			case F_SETFL:
			case F_SETOWN:
#ifdef F_SETSIG
			case F_SETSIG:
#endif
#ifdef F_SETLEASE
			case F_SETLEASE:
#endif
#ifdef F_NOTIFY
			case F_NOTIFY:
#endif
				res = ::fcntl(m_fd,cmd,int(l.checkinteger(3)));
				break;
			default: {
				l.pushnil();
				l.pushstring("unsupported");
				return {2};
			} break;
		}
		if (res < 0) {
			l.pushnil();
			push_error_errno(l);
			return {2};
		}
		l.pushinteger(res);
		return {1};
	}

	void fd::lbind(lua::state& l) {
		lua::bind::function(l,"close",&fd::close);
		lua::bind::function(l,"read",&fd::read);
		lua::bind::function(l,"write",&fd::write);
		lua::bind::function(l,"fcntl",&fd::fcntl);
	}

}