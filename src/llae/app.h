#ifndef __LLAE_LLAE_H_INCLUDED__
#define __LLAE_LLAE_H_INCLUDED__

#include "lua/state.h"
#include "uv/loop.h"
#include "uv/signal.h"
#include "common/intrusive_ptr.h"

namespace llae {

	class app {
		lua::main_state m_lua;
		uv::loop 	m_loop;
		common::intrusive_ptr<uv::signal> m_stop_sig;
	public:
		explicit app(uv_loop_t* l);
		~app();

		lua::state& lua() { return m_lua; }
		uv::loop& loop() { return m_loop; }

		void run();
		void stop();
		static void show_error(lua::state& l,lua::status e);
		
        static bool closed(uv_loop_t* l);
		static app& get(lua_State* L);
		static app& get(uv_loop_t* l);
		static app& get(lua::state& s) { return get(s.native()); }
		static app& get(uv::loop& l) { return get(l.native()); }
	};

}

int luaopen_llae(lua_State* L);


#endif /*__LLAE_LLAE_H_INCLUDED__*/
