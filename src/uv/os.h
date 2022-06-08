#ifndef __LLAE_UV_OS_H_INCLUDED__
#define __LLAE_UV_OS_H_INCLUDED__

#include "lua/state.h"

namespace uv {
	struct os {
		static int homedir(lua_State* L);
		static int tmpdir(lua_State* L);
		static int getenv(lua_State* L);
		static int setenv(lua_State* L);
		static int unsetenv(lua_State* L);
		static int gethostname(lua_State* L);
		static int uname(lua_State* L);
		static int getpriority(lua_State* L);
		static int setpriority(lua_State* L);
		static int getpid(lua_State* L);
		static void lbind(lua::state& l);
	};
}

#endif /*__LLAE_UV_OS_H_INCLUDED__*/