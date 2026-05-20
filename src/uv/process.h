#ifndef __LLAE_UV_PROCESS_H_INCLUDED__
#define __LLAE_UV_PROCESS_H_INCLUDED__

#include "handle.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "common/intrusive_ptr.h"

#include <string>
#include <vector>

namespace uv {

	class loop;

	class process;
	
	/// @luabind
	class process : public handle {
		META_OBJECT
	private:
		uv_process_t m_process;
		lua::ref m_cont;
        bool m_completed = false;
        int64_t m_exit_status = 0;
        int m_term_signal = 0;
        std::string m_cwd;
        std::vector<std::string> m_env;
        std::vector<char*> m_env_list;
        std::vector<std::string> m_args;
        std::vector<char*> m_args_list;
        std::vector<uv_stdio_container_s> m_streams;
        bool load_args(lua::state& l,uv_process_options_t& optrions,lua::multiret res);
		static void on_exit_cb(uv_process_t*, int64_t exit_status, int term_signal);
		void on_exit(int64_t exit_status, int term_signal);
	public:
		virtual uv_handle_t* get_handle() override final { return reinterpret_cast<uv_handle_t*>(&m_process); }
	protected:
		virtual ~process() override;
		virtual void on_closed() override;
		explicit process();
		int do_spawn(loop& l,const uv_process_options_t *options);
	public:
		
		/// @luabind
		static lua::multiret spawn(lua::state& l);
		/// @luabind
		lua::multiret kill(lua::state& l,int signal);
        /// @luabind
        lua::multiret wait_exit(lua::state& l);

#ifdef LUABIND_PARSE
		/// @luabind(ltype=integer,value=UV_IGNORE)
		static constexpr auto IGNORE = UV_IGNORE;
		/// @luabind(ltype=integer,value=UV_CREATE_PIPE)
		static constexpr auto CREATE_PIPE = UV_CREATE_PIPE;
		/// @luabind(ltype=integer,value=UV_INHERIT_FD)
		static constexpr auto INHERIT_FD = UV_INHERIT_FD;
		/// @luabind(ltype=integer,value=UV_INHERIT_STREAM)
		static constexpr auto INHERIT_STREAM = UV_INHERIT_STREAM;
		/// @luabind(ltype=integer,value=UV_READABLE_PIPE)
		static constexpr auto READABLE_PIPE = UV_READABLE_PIPE;
		/// @luabind(ltype=integer,value=UV_WRITABLE_PIPE)
		static constexpr auto WRITABLE_PIPE = UV_WRITABLE_PIPE;
		/// @luabind(ltype=integer,value=UV_NONBLOCK_PIPE)
		static constexpr auto NONBLOCK_PIPE = UV_NONBLOCK_PIPE;
		/// @luabind(ltype=integer,value=UV_PROCESS_DETACHED)
		static constexpr auto PROCESS_DETACHED = UV_PROCESS_DETACHED;
#endif
	};
	typedef common::intrusive_ptr<process> process_ptr;

}

#endif /*__LLAE_UV_PROCESS_H_INCLUDED__*/
