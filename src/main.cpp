#include <iostream>
#include <string>

#include "llae.h"

extern "C" int luaopen_llae_crypto(lua_State* L);
extern "C" int luaopen_llae_json(lua_State* L);
extern "C" int luaopen_llae(lua_State* L);

static void createargtable (lua_State *L, char **argv, int argc, int script) {
  if (script == argc) script = 0;  /* no script name? */
  int narg = argc - (script + 1);  /* number of positive indices */
  lua_createtable(L, narg, script + 1);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - script);
  }
  lua_setglobal(L, "args");
}

extern "C" void llae_register_modules(lua_State *L) __attribute__((weak));

__attribute__((weak)) void llae_register_modules(lua_State *L) {

}


int main(int argc,char** argv) {
	if (argc < 2) {
		std::cout << "usage: " << argv[0] << " file.lua [args]" << std::endl;
		return 1;
	}

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	
	static const luaL_Reg loadedlibs[] = {
      {"llae.crypto", luaopen_llae_crypto},
      {"llae.json", luaopen_llae_json},
      {"llae",luaopen_llae},
      {NULL, NULL}
  };
    
  const luaL_Reg *lib;
  /* call open functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
      luaL_requiref(L, lib->name, lib->func, 1);
      lua_pop(L, 1);  /* remove lib */
  }

  llae_register_modules(L);

	std::string file = argv[1];
	createargtable(L,argv,argc,1);

	if (luaL_dofile(L,file.c_str())) {
		std::cout << "failed run " << file << std::endl;
		if (lua_isstring(L,-1)) {
			std::cout << lua_tostring(L,-1) << std::endl;
		}
		return 1;
	}

	lua_close(L);

	uv_print_all_handles(uv_default_loop(), stderr);

	uv_loop_close(uv_default_loop());

	return 0;
}
