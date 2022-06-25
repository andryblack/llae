#include "posix/fd.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "lua/raw_bind.h"
#include "lua/bind.h"
#include "posix/lposix.h"

#include <ctime>


int luaopen_posix_time(lua_State* L) {
	lua::state l(L);
	
	l.createtable();


	return 1;
}

