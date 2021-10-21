#ifndef __LLAE_LLAE_H_INCLUDED__
#define __LLAE_LLAE_H_INCLUDED__

#include "lua/state.h"
#include "uv/loop.h"
#include "uv/signal.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/ref.h"

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

		template <typename ...Args>
		void resume(lua::ref& cont,Args&&... args) {
			auto& l(lua());
            cont.push(l);
			cont.reset(l);
			auto toth = l.tothread(-1);
			toth.checkstack(sizeof...(Args));
			int unused[]{(lua::push(toth,std::move(args)),1)...};
            (void)unused;
			auto s = toth.resume(l,sizeof...(Args));
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			l.pop(1);// thread
		}
	};

}

int luaopen_llae(lua_State* L);


#endif /*__LLAE_LLAE_H_INCLUDED__*/
