#include "embedded.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

__attribute__((weak)) const lua::embedded_script lua::embedded_script::scripts[] = {
	{0,0,0}
};

namespace lua {

	static int load_script(lua_State* L,const embedded_script* script) {
		lua_pushstring(L, "$");
		lua_pushstring(L, script->name);
		lua_pushstring(L, ".lua");
		lua_concat(L, 3);
		luaL_loadbuffer(L, (const char*)script->content, script->size, script->name);
		return 1;
	}

	static int embedded_searcher(lua_State* L) {
		const char *name = luaL_checkstring(L, 1);
		const embedded_script* s = embedded_script::scripts;
		while (s->name) {
			if (strcmp(s->name,name)==0){
				return load_script(L,s);
			}
			++s;
		}
		lua_pushstring(L,"embedded");
		return 1;
	}

	static int get_embedded(lua_State* L) {
		const char *name = luaL_checkstring(L, 1);
		const embedded_script* s = embedded_script::scripts;
		while (s->name) {
			if (strcmp(s->name,name)==0){
				lua_pushlstring(L,(const char*)s->content, s->size);
				return 1;
			}
			++s;
		}
		return 0;
	}


	void attach_embedded_scripts(lua::state& lua) {
		lua.getglobal("table");
		lua.getfield(-1,"insert");
		lua.remove(-2);
		lua.getglobal("package");
		lua.getfield(-1,"searchers");
		lua.remove(-2);
		lua.pushinteger(1);
		lua.pushcclosure(embedded_searcher,0);
		lua.call(3,0);

		lua.getglobal("package");
		lua.pushcfunction(&get_embedded);
		lua.setfield(-2,"get_embedded");
		lua.pop(1);
	}

}