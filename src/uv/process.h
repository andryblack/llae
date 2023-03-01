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
		static void lbind(lua::state& l);
		
		static lua::multiret spawn(lua::state& l);
		lua::multiret kill(lua::state& l,int signal);
        lua::multiret wait_exit(lua::state& l);
	};
	typedef common::intrusive_ptr<process> process_ptr;

}

#endif /*__LLAE_UV_PROCESS_H_INCLUDED__*/
