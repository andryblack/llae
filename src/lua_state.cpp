#include "lua_state.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <memory>

namespace Lua {

	Status StateRef::dostring(const char* str) {
		int r = luaL_dostring(m_L,str);
		return static_cast<Status>(r);
	}
	void StateRef::require(const char* name,lua_CFunction openfunc) {
		luaL_requiref(m_L,name,openfunc,0);
		lua_pop(m_L, 1);  /* remove lib */
	}

	int StateRef::ref() {
		return luaL_ref(m_L,LUA_REGISTRYINDEX);
	}
	void StateRef::unref(int r) {
		luaL_unref(m_L,LUA_REGISTRYINDEX,r);
	}

	State::State() : StateRef( 0 ) {
		m_L = lua_newstate( &State::lua_alloc, this );
	}

	State::~State() {
		close();
	}



	void *State::lua_alloc (void *ud, void *ptr, size_t osize,
	                                                size_t nsize) {
		if (nsize == 0) {
			::free(ptr);
			return 0;
		}
		if (!ptr || !osize) {
			return ::malloc(nsize);
		}
		return ::realloc(ptr,nsize);

	}

	void State::open_libs() {
		luaL_openlibs(m_L);
	}

	void State::close() {
		if (m_L) {
			lua_close(m_L);
			m_L = 0;
		}
	}

}