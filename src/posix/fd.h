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
		int close();
		ssize_t read(void* data,size_t size);
		ssize_t write(const void* data,size_t size);

		lua::multiret lclose(lua::state& l);
		lua::multiret lread(lua::state& l);
		lua::multiret lwrite(lua::state& l);
		lua::multiret fcntl(lua::state& l);

		static void lbind(lua::state& l);
	};
	using fd_ptr = common::intrusive_ptr<fd>;

}

#endif /*__LLAE_POSIX_FD_H_INCLUDED__*/