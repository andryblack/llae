#include <iostream>
#include <string>

#include "lua/state.h"
#include "lua/embedded.h"
#include "lua/value.h"
#include "uv/loop.h"
#include "uv/handle.h"
#include "llae/app.h"

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
	auto loop = uv_default_loop();
    {
		llae::app app{loop};

		lua::state& L(app.lua());

		/* call open functions from 'loadedlibs' and set results to global table */
		for (const auto *lib = embedded_libs; lib->func; lib++) {
			L.require(lib->name,lib->func);
		}

		lua::attach_embedded_scripts(L);
		L.getglobal("require");
		L.pushstring("main");
		auto err = L.pcall(1,1,0);
		if (err != lua::status::ok) {
			app.show_error(L,err);
		} else if (!lua::value(L,-1).is_function()) {
			std::cout << "main dnt return function" << std::endl;
		} else {
			createargtable(L,argv,argc);
			err = L.pcall(1,0,0);
			if (err != lua::status::ok) {
				app.show_error(L,err);
			} else {
				app.run();
			}
		}	
	}

	while (uv_loop_close(loop) == UV_EBUSY) {
		uv_sleep(100);
	}

	std::cout << "meta objects: " << meta::object::get_total_count() << std::endl;

	return 0;
}
