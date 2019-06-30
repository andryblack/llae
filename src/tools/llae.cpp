#include <iostream>
#include <string>

#include "../llae.h"
#include "../lua_state.h"
#include "../lua_value.h"

extern "C" int luaopen_llae_crypto(lua_State* L);
extern "C" int luaopen_llae_json(lua_State* L);
extern "C" int luaopen_llae(lua_State* L);
extern "C" int luaopen_llae_file(lua_State* L);

int attach_embedded_scripts(Lua::StateRef& lua);

static void createargtable (Lua::StateRef& lua, char **argv, int argc) {
  int narg = argc - 1;  /* number of positive indices */
	lua.createtable(narg,1);
	for (int i = 0; i < argc; i++) {
    	lua.pushstring(argv[i]);
    	lua.rawseti(-2, i);
  	}
  	lua.setglobal("args");
}


int main(int argc,char** argv) {

	Lua::State lua;

	lua.open_libs();

	static const struct RegLib{
		const char* name;
		lua_CFunction func;
	} loadedlibs [] = {
      {"llae.crypto", luaopen_llae_crypto},
      {"llae.json", luaopen_llae_json},
      {"llae",luaopen_llae},
      {"llae.file",luaopen_llae_file},
      {NULL, NULL}
  	};
    
	const RegLib *lib;
	/* call open functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++) {
		lua.require(lib->name,lib->func);
	}

	attach_embedded_scripts(lua);

	createargtable(lua,argv,argc);

	Lua::Status status = lua.dostring("(require 'main')(args);");
	if (status != Lua::Status::OK) {
		Lua::StackValue err(lua,-1);
		if (err.is_string()) {
			std::cout << err.tostring() << std::endl;
		} else {
			std::cout << "failed run main" << std::endl;
		}
	}

	lua.close();

	uv_loop_close(uv_default_loop());

	return 0;
}
