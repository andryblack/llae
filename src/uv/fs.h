#ifndef __LLAE_UV_FS_H_INCLUDED__
#define __LLAE_UV_FS_H_INCLUDED__

#include "llae/promise.h"
#include "req.h"
#include "loop.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/buffer.h"
#include "common/intrusive_ptr.h"
#include "meta/object.h"
#include "llae/promise.h"
#include <optional>

namespace llae {
	class buffer_view;
}

namespace uv {

	

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
		/// @luabind(async=true,name=close)
		llae::result_promise_ptr<void> async_close(llae::loop& l);
		/// @luabind(async=true,name=fsync)
		llae::result_promise_ptr<void> async_fsync(llae::loop& l);
		/// @luabind(async=true,name=write)
		llae::result_promise_ptr<void> lasync_write(lua::state& l);
        /// @luabind(async=true,name=read)
        llae::result_promise_ptr<llae::buffer_ptr> async_read(llae::loop& l,std::optional<size_t> size);
        /// @luabind
        void seek(size_t pos) { m_offset = pos; }
	    int64_t get_offset() const { return m_offset;}
        /// @luabind(name=tell)
        size_t tell() const { return m_offset; }
		llae::result_promise_ptr<void> async_write(llae::loop& l,const llae::buffer_view& data);
	};
	typedef common::intrusive_ptr<file> file_ptr;

	struct dirent_t {
		std::string name;
		uv_dirent_type_t type;
	};
	
	namespace fs {
		/// @luabind(async=true,name=mkdir)
		llae::result_promise_ptr<void> async_mkdir(llae::loop& l,std::string_view path,std::optional<int> mode);
		/// @luabind(async=true,name=rmdir)
		llae::result_promise_ptr<void> async_rmdir(llae::loop& l,std::string_view path);
		/// @luabind(async=true,name=unlink)
		llae::result_promise_ptr<void> async_unlink(llae::loop& l,std::string_view path);
		/// @luabind(async=true,name=copyfile)
		llae::result_promise_ptr<void> async_copyfile(llae::loop& l,std::string_view path,std::string_view new_path,std::optional<int> flags);
		/// @luabind(async=true,name=rename)
		llae::result_promise_ptr<void> async_rename(llae::loop& l,std::string_view path,std::string_view new_path);
		/// @luabind(async=true,name=stat)
		llae::result_promise_ptr<uv_stat_t> async_stat(llae::loop& l,std::string_view path);
		/// @luabind(async=true,name=scandir)
		llae::result_promise_ptr<std::vector<dirent_t>> async_scandir(llae::loop& l,std::string_view path,std::optional<int> flags);
		/// @luabind(async=true,name=open)
		llae::result_promise_ptr<file_ptr> async_open(llae::loop& l,std::string_view path,std::optional<int> flags,std::optional<int> mode);
		/// @luabind(async=true,name=chmod)
		llae::result_promise_ptr<void> async_chmod(llae::loop& l,std::string_view path,int mode);

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

namespace lua {
	template <>
	struct stack<uv_stat_t> {
		static int push(lua::state& l,const uv_stat_t& stat);
	};
	template <>
	struct stack<uv::dirent_t> {
		static int push(lua::state& l,const uv::dirent_t& ent);
	};
	template <>
	struct stack<std::vector<uv::dirent_t>> {
		static int push(lua::state& l,const std::vector<uv::dirent_t>& ent);
	};
}
#endif /*__LLAE_UV_FS_H_INCLUDED__*/
