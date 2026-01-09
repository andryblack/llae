#include "app.h"
#include "buffer.h"
#include "lua/bind.h"
#include "logger.h"
#include "uv/fs.h"

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
        auto holder = lua::object_holder_base_t::get(s,1);
        if (holder) {
            holder->release();
        }
        return 0;
    }

	static int lua_stop(lua_State* L) {
        app::get(L).stop(static_cast<int>(luaL_optinteger(L,1,0)));
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

static void log_handler_bind(lua::state& l) {
    lua::bind::function(l,"close",llae::log::close);
}

static void set_error_handler(lua::state& l) {
    auto handler = lua::stack<llae::error_handler_ptr>::get(l,1);
    llae::app::get(l).set_error_handler(handler);
}

int luaopen_llae(lua_State* L) {

   

    lua::state l(L);

    lua::bind::object<llae::buffer_base>::register_metatable(l,&llae::buffer_base::lbind);
	lua::bind::object<llae::buffer>::register_metatable(l,&llae::buffer::lbind);
    lua::bind::object<llae::writable_buffer>::register_metatable(l,&llae::writable_buffer::lbind);
    lua::bind::object<llae::log_handler>::register_metatable(l,&log_handler_bind);
    
    l.createtable();
    lua::bind::function(l, "stop", llae::lua_stop );
    lua::bind::function(l, "at_exit", llae::lua_at_exit );
    lua::bind::function(l, "cancel_sigint", llae::lua_cancel_signal );
    lua::bind::function(l, "release_object", llae::lua_release_object );
    lua::bind::function(l, "resume", llae::lua_resume );
    lua::bind::function(l, "get_host_platform", llae::lua_get_host_platform );
    lua::bind::function(l, "set_error_handler", set_error_handler );

    lua::bind::object<llae::buffer>::get_metatable(l);
	l.setfield(-2,"buffer");
    lua::bind::object<llae::writable_buffer>::get_metatable(l);
	l.setfield(-2,"writable_buffer");
	
    l.createtable();
    l.createtable();
    lua::bind::value(l,"debug",llae::log::level::debug);
    lua::bind::value(l,"info",llae::log::level::info);
    lua::bind::value(l,"warning",llae::log::level::warning);
    lua::bind::value(l,"error",llae::log::level::error);
    lua::bind::value(l,"fatal",llae::log::level::fatal);
    l.setfield(-2,"level");
    lua::bind::function(l,"write",llae::log::write);
    lua::bind::function(l,"add_stdout_handler",llae::log::add_stdout_handler);
    lua::bind::function(l,"remove_stdout_handler",llae::log::remove_stdout_handler);
    lua::bind::function(l,"add_file_handler",llae::log::add_file_handler);
    l.setfield(-2,"log");

    return 1;
}
