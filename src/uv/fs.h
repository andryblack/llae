#ifndef __LLAE_UV_FS_H_INCLUDED__
#define __LLAE_UV_FS_H_INCLUDED__

#include "req.h"
#include "loop.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {

	class fs : public req {
		META_OBJECT
	private:
		uv_fs_t	m_fs;
		lua::ref m_cont;
	protected:
		uv_fs_t* fs_req() { return &m_fs; }
		static void fs_cb(uv_fs_t* req);
		static fs* get(uv_fs_t* req);
	protected:
		fs(lua::ref&& cont);
		~fs();
		virtual int on_cb(lua::state& l) = 0;
	public:
		static int mkdir(lua_State* L);
		static int copyfile(lua_State* L);
	};
}

#endif /*__LLAE_UV_FS_H_INCLUDED__*/