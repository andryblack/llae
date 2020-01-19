#include "loop.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"



int luaopen_uv(lua_State* L) {
	lua::state s(L);
	s.createtable();
	return 1;
}