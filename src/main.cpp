#include <iostream>
#include <string>

#include "lua/state.h"
#include "lua/embedded.h"
#include "lua/value.h"
#include "uv/loop.h"
#include "uv/handle.h"
#include "llae/app.h"
#include "llae/diag.h"
#include "uv/buffer.h"

#ifndef WIN32
#include <signal.h>
#include <sys/resource.h>
#endif

static void createargtable (lua::state& lua, char **argv, int argc) {
  	int narg = argc - 1;  /* number of positive indices */
	lua.createtable(narg,1);
	for (int i = 0; i < argc; i++) {
    	lua.pushstring(argv[i]);
    	lua.rawseti(-2, i);
  	}
}

static int err_handler(lua_State* L) {
	auto msg = lua_tostring(L,-1);
	luaL_traceback(L,L,msg,1);
	return 1;
}

int main(int argc,char** argv) {
#ifndef WIN32
	signal(SIGPIPE,SIG_IGN);

	// reduce uv threads stack size
	struct rlimit lim;
	if (0 == getrlimit(RLIMIT_STACK, &lim) && lim.rlim_cur != RLIM_INFINITY) {
		lim.rlim_cur = 1 << 20;
		setrlimit(RLIMIT_STACK,&lim);
	}
#endif
	auto loop = uv_default_loop();
	int retcode = 0;
    {
		llae::app app{loop};

		lua::state& L(app.lua());

		lua::attach_embedded_modules(L);

		lua::attach_embedded_scripts(L);

		L.pushcfunction(&err_handler);
		auto err = lua::load_embedded(L,"_main");
		if (err!=lua::status::ok) {
			app.show_error(L,err);
			retcode = 1;
		} else {
			createargtable(L,argv,argc);
			err = L.pcall(1,0,-3);
			if (err != lua::status::ok) {
				app.show_error(L,err);
                app.stop();
                retcode = 1;
			}
            app.run();
		}
	}

	size_t wait_cnt = 0;
    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
        uv_print_all_handles(loop, stderr);
        if (++wait_cnt > 31) {
        	break;
        }
    }

	LLAE_DIAG(std::cout << "meta objects:  " << meta::object::get_total_count() << std::endl;)
	LLAE_DIAG(std::cout << "buffers alloc: " << llae::named_alloc<uv::buffer>::get_allocated() << std::endl;)

	return retcode;
}
