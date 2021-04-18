#include "embedded.h"
#include "archive/archive.h"
#include <memory>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace lua {

	static int load_script(lua_State* L,const embedded_script* script) {
		size_t uncompressed = script->uncompressed;
		std::string chname = "embedded:";
		chname.append(script->name);
		if (uncompressed) {
			std::unique_ptr<char[]> data(new char[script->uncompressed]);
			if (archive::inflate(script->content,script->size,data.get(),uncompressed)) {
				return luaL_loadbuffer(L, data.get(), uncompressed, chname.c_str());
			} else {
				return LUA_ERRRUN;
			}
		} 

		return luaL_loadbuffer(L, reinterpret_cast<const char*>(script->content), script->size, chname.c_str());
	}

	static int embedded_searcher(lua_State* L) {
		const char *name = luaL_checkstring(L, 1);
		const embedded_script* s = embedded_script::scripts;
		while (s->name) {
			if (strcmp(s->name,name)==0){
				int err = load_script(L,s);
				if (err!=LUA_OK) {
					luaL_error(L,"filed load embedded script %s : %d",s->name,err);
				}
				return 1;
			}
			++s;
		}
		lua_pushstring(L,"embedded");
		return 1;
	}

	static const embedded_script* find_embedded(const char* name) {
		const embedded_script* s = embedded_script::scripts;
		while (s->name) {
			if (strcmp(s->name,name)==0){
				return s;
			}
			++s;
		}
		return nullptr;
	}

	static int get_embedded(lua_State* L) {
		const char *name = luaL_checkstring(L, 1);
		const embedded_script* s = find_embedded(name);
		if (s) {
			size_t uncompressed = s->uncompressed;
			if (uncompressed) {
				std::unique_ptr<char[]> data(new char[s->uncompressed]);
				if (!archive::inflate(s->content,s->size,data.get(),uncompressed)) {
					return 0;
				}
				lua_pushlstring(L,data.get(), uncompressed);
			} else {
				lua_pushlstring(L,reinterpret_cast<const char*>(s->content), s->size);
			}
			return 1;
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

	status load_embedded(lua::state& l,const char* name) {
		const embedded_script* s = find_embedded(name);
		if (!s) {
			l.pushstring("not found embedded ");
			l.pushstring(name);
			l.concat(2);
			return status::errun;
		}
		int res = load_script(l.native(), s);
		return static_cast<status>(res);
	}

	void attach_embedded_modules(lua::state& l) {
		/* call open functions from 'loadedlibs' and set results to global table */
		for (const auto *lib = embedded_module::modules; lib->func; lib++) {
			l.require(lib->name,lib->func);
		}
	}

}