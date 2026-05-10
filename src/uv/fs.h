#ifndef __LLAE_UV_FS_H_INCLUDED__
#define __LLAE_UV_FS_H_INCLUDED__

#include "req.h"
#include "loop.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"
#include "meta/object.h"

namespace llae {
	class buffer_view;
}

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
	using fs_req_ptr = common::intrusive_ptr<fs_req>;

	/// @luabind
	class file : public meta::object {
		META_OBJECT
	private:
		uv_file m_file;
		uv_loop_t* m_loop;
		int64_t m_offset = 0;
		virtual void destroy() override final;
	public:
		explicit file(uv_file f,uv_loop_t* l);
		/// @luabind(name=get_handle)
		uv_file get() const { return m_file; }
		/// @luabind(name=close)
		lua::multiret fclose(lua::state& l);
		/// @luabind(name=write)
		lua::multiret lwrite(lua::state& l);
        /// @luabind
        lua::multiret read(lua::state& l);
        /// @luabind
        void seek(size_t pos) { m_offset = pos; }
        int64_t get_offset() const { return m_offset;}
        /// @luabind(name=tell)
        size_t tell() const { return m_offset; }
		fs_req_ptr write(loop& l,const llae::buffer_view& data);
		fs_req_ptr close(loop& l);
		fs_req_ptr fsync(loop& l);
	};
	typedef common::intrusive_ptr<file> file_ptr;

	
	namespace fs {
		/// @luabind
		int mkdir(lua_State* L);
		/// @luabind
		int rmdir(lua_State* L);
		/// @luabind
		int unlink(lua_State* L);
		/// @luabind
		int copyfile(lua_State* L);
		/// @luabind
		int rename(lua_State* L);
		/// @luabind
		int stat(lua_State* L);
		/// @luabind
		int scandir(lua_State* L);
		/// @luabind
		int open(lua_State* L);
		/// @luabind
		int chmod(lua_State* L);

		/// @luabind(ltype=integer,name=O_RDONLY)
		static constexpr auto LO_RDONLY = UV_FS_O_RDONLY;
		/// @luabind(ltype=integer,name=O_RDWR)
		static constexpr auto LO_RDWR = UV_FS_O_RDWR;
		/// @luabind(ltype=integer,name=O_WRONLY)
		static constexpr auto LO_WRONLY = UV_FS_O_WRONLY;
		/// @luabind(ltype=integer,name=O_CREAT)
		static constexpr auto LO_CREAT = UV_FS_O_CREAT;
		/// @luabind(ltype=integer,name=O_APPEND)
		static constexpr auto LO_APPEND = UV_FS_O_APPEND;
	};
}

#endif /*__LLAE_UV_FS_H_INCLUDED__*/
