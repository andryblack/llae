#ifndef __LLAE_UV_OS_H_INCLUDED__
#define __LLAE_UV_OS_H_INCLUDED__

#include "lua/state.h"

namespace uv {
	namespace os {
		/// @luabind
		int homedir(lua_State* L);
		/// @luabind
		int tmpdir(lua_State* L);
		/// @luabind
		int getenv(lua_State* L);
		/// @luabind
		int setenv(lua_State* L);
		/// @luabind
		int getallenv(lua_State* L);
		/// @luabind
		int unsetenv(lua_State* L);
		/// @luabind
		int gethostname(lua_State* L);
		/// @luabind
		int uname(lua_State* L);
		/// @luabind
		int getpriority(lua_State* L);
		/// @luabind
		int setpriority(lua_State* L);
		/// @luabind
		int getpid(lua_State* L);
	};
}

#endif /*__LLAE_UV_OS_H_INCLUDED__*/