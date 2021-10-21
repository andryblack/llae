#ifndef __LLAE_POSIX_FD_H_INCLUDED__
#define __LLAE_POSIX_FD_H_INCLUDED__

#include "lua/ref.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace posix {


	class fd : public meta::object  {
		META_OBJECT
	private:
		int m_fd = -1;
	public:
		explicit fd(int f) : m_fd(f) {}
		~fd();
		int get() const { return m_fd; }
		lua::multiret close(lua::state& l);
		lua::multiret read(lua::state& l);
		lua::multiret write(lua::state& l);
		lua::multiret fcntl(lua::state& l);

		static void lbind(lua::state& l);
	};
	using fd_ptr = common::intrusive_ptr<fd>;

}

#endif /*__LLAE_POSIX_FD_H_INCLUDED__*/