#ifndef __LLAE_UV_FS_H_INCLUDED__
#define __LLAE_UV_FS_H_INCLUDED__

#include "req.h"
#include "loop.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"

namespace uv {

	class fs_req : public req {
	private:
		uv_fs_t	m_fs;
	protected:
		static fs_req* get(uv_fs_t* req);
	protected:
		fs_req();
		~fs_req();
		virtual void on_cb() = 0;
	public:
		uv_fs_t* get() { return &m_fs; }
		static void fs_cb(uv_fs_t* req);
	};

	class file : public meta::object {
		META_OBJECT
	private:
		uv_file m_file;
		uv_loop_t* m_loop;
		int64_t m_offset = 0;
		virtual void destroy() override final;
	public:
		explicit file(uv_file f,uv_loop_t* l);
		uv_file get() const { return m_file; }
		static void lbind(lua::state& l);
		lua::multiret close(lua::state& l);
		lua::multiret write(lua::state& l);
        lua::multiret read(lua::state& l);
        void seek(size_t pos) { m_offset = pos; }
	};
	typedef common::intrusive_ptr<file> file_ptr;

	
	struct fs {
		static int mkdir(lua_State* L);
		static int rmdir(lua_State* L);
		static int unlink(lua_State* L);
		static int copyfile(lua_State* L);
		static int rename(lua_State* L);
		static int stat(lua_State* L);
		static int scandir(lua_State* L);
		static int open(lua_State* L);
		static void lbind(lua::state& l);
	};
}

#endif /*__LLAE_UV_FS_H_INCLUDED__*/
