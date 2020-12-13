#include <iostream>
#include <string>

#include "lua/state.h"
#include "lua/embedded.h"
#include "lua/value.h"
#include "uv/loop.h"
#include "uv/handle.h"
#include "llae/app.h"
#include "llae/diag.h"

extern "C" int luaopen_llae_crypto(lua_State* L);
extern "C" int luaopen_llae_file(lua_State* L);

int luaopen_json(lua_State* L);
int luaopen_uv(lua_State* L);
int luaopen_ssl(lua_State* L);
int luaopen_llae(lua_State* L);
int luaopen_archive(lua_State* L);

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
	{"ssl",		luaopen_ssl},
	{"json",	luaopen_json},
	{"llae",	luaopen_llae},
	{"archive",	luaopen_archive},
  {NULL, NULL}
};

static int err_handler(lua_State* L) {
	auto msg = lua_tostring(L,-1);
	luaL_traceback(L,L,msg,1);
	return 1;
}

extern "C" 
__attribute__((weak)) void llae_register_modules(lua_State *L) {

}

int main(int argc,char** argv) {
	auto loop = uv_default_loop();
    {
		llae::app app{loop};

		lua::state& L(app.lua());

		/* call open functions from 'loadedlibs' and set results to global table */
		for (const auto *lib = embedded_libs; lib->func; lib++) {
			L.require(lib->name,lib->func);
		}

		llae_register_modules(L.native());

		lua::attach_embedded_scripts(L);
		L.pushcfunction(&err_handler);
		L.getglobal("require");
		L.pushstring("_main");
		auto err = L.pcall(1,1,-3);
		if (err != lua::status::ok) {
			app.show_error(L,err);
		} else if (!lua::value(L,-1).is_function()) {
			std::cout << "main dnt return function" << std::endl;
		} else {
			createargtable(L,argv,argc);
			err = L.pcall(1,0,-3);
			if (err != lua::status::ok) {
				app.show_error(L,err);
			} else {
				app.run();
			}
		}
	}

    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
        uv_print_all_handles(loop, stderr);
    }

	LLAE_DIAG(std::cout << "meta objects: " << meta::object::get_total_count() << std::endl;)

	return 0;
}
