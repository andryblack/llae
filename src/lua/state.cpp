#include "lua/state.h"
#include <memory>
#include <cstdlib>

namespace lua {

	status state::dostring(const char* str) {
		int r = luaL_dostring(m_L,str);
		return static_cast<status>(r);
	}
	void state::require(const char* name,lua_CFunction openfunc) {
		luaL_requiref(m_L,name,openfunc,0);
		lua_pop(m_L, 1);  /* remove lib */
	}

	main_state::main_state() : state( 0 ) {
		m_L = lua_newstate( &main_state::lua_alloc, this );
	}

	main_state::~main_state() {
		close();
	}

	void *main_state::lua_alloc (void *ud, void *ptr, size_t osize,
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

	void main_state::open_libs() {
		luaL_openlibs(m_L);
	}

	void main_state::close() {
		if (m_L) {
			lua_close(m_L);
			m_L = 0;
		}
	}

}