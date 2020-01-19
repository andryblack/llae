#include <iostream>
#include <string>

#include "lua/state.h"
#include "lua/embedded.h"
#include "lua/value.h"
#include "uv/loop.h"

extern "C" int luaopen_llae_crypto(lua_State* L);

extern "C" int luaopen_llae_file(lua_State* L);

int luaopen_json(lua_State* L);
int luaopen_uv(lua_State* L);
int luaopen_llae(lua_State* L);


static void createargtable (lua::state& lua, char **argv, int argc) {
  int narg = argc - 1;  /* number of positive indices */
	lua.createtable(narg,1);
	for (int i = 0; i < argc; i++) {
    	lua.pushstring(argv[i]);
    	lua.rawseti(-2, i);
  	}
  	lua.setglobal("args");
}

static const struct {
		const char* name;
		lua_CFunction func;
} embedded_libs [] = {
  //{"llae.crypto", luaopen_llae_crypto},
  //{"llae.json", luaopen_llae_json},
  //{"llae",luaopen_llae},
  //{"llae.file",luaopen_llae_file},
	{"uv",		luaopen_uv},
	{"json",	luaopen_json},
	{"llae",	luaopen_llae},
  {NULL, NULL}
};

int main(int argc,char** argv) {

	lua::main_state lua;

	lua.open_libs();

	/* call open functions from 'loadedlibs' and set results to global table */
	for (const auto *lib = embedded_libs; lib->func; lib++) {
		lua.require(lib->name,lib->func);
	}

	lua::attach_embedded_scripts(lua);

	createargtable(lua,argv,argc);

	lua::status status = lua.dostring("(require 'main')(args);");
	if (status != lua::status::ok) {
		lua::value err(lua,-1);
		if (err.is_string()) {
			std::cout << err.tostring() << std::endl;
		} else {
			std::cout << "failed run main" << std::endl;
		}
	}

	lua.close();

	//uv_loop_close(uv_default_loop());

	return 0;
}
