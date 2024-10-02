#include "app.h"

#if defined(__APPLE__)
/* lets us know what version of Mac OS X we're compiling on */
#include <AvailabilityMacros.h>
#ifndef __has_extension /* Older compilers don't support this */
#define __has_extension(x) 0
#include <TargetConditionals.h>
#undef __has_extension
#else
#include <TargetConditionals.h>
#endif
#endif

namespace llae {

	

    static int lua_release_object(lua_State* L) {
        lua::state s(L);
        auto holder = lua::object_holder_t::get(s,1);
        if (holder) {
            holder->hold.reset();
        }
        return 0;
    }

	static int lua_stop(lua_State* L) {
        app::get(L).stop(luaL_optinteger(L,1,0));
        return 0;
    }

    static int lua_at_exit(lua_State* L) {
        lua::state l(L);
        l.checktype(1,lua::value_type::function);
        app::get(l).at_exit(l,1);
        return 0;
    }

    static int lua_cancel_signal(lua_State* L) {
        app::get(L).cancel_signal();
        return 0;
    }
 
    static int lua_resume(lua_State* L) {
        lua::state l(L);
        app::lua_resume(l);
        return 0;
    }   

    static int lua_get_host_platform(lua_State* L) {
    	lua::state l(L);
#if defined(__linux__)
    	l.pushstring("linux");
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    	l.pushstring("bsd");
#elif defined(__APPLE__) 
	#if TARGET_OS_TV
    	l.pushstring("tvos");
	#elif TARGET_OS_IPHONE
    	l.pushstring("ios");
	#elif TARGET_OS_MAC
    	l.pushstring("macosx");
	#else
    	l.pushstring("apple");
	#endif
#elif defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
    	l.pushstring("windows");
#else
    	l.pushstring("unknown");
#endif
    	return 1;
    }
}

int luaopen_llae(lua_State* L) {
    lua::state s(L);
    luaL_Reg reg[] = {
        { "stop", llae::lua_stop },
        { "at_exit", llae::lua_at_exit },
        { "cancel_sigint", llae::lua_cancel_signal },
        { "release_object", llae::lua_release_object },
        { "resume", llae::lua_resume },
        { "get_host_platform", llae::lua_get_host_platform },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}
