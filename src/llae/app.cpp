#include "app.h"
#include "lua/value.h"
#include "meta/object.h"
#include "lua/bind.h"
#include <iostream>

namespace llae {

    static int at_panic(lua_State* L) {
        std::cout << "PANIC: ";
        lua::state l(L);
        lua::value err(l,-1);
        if (err.is_string()) {
            std::cout << err.tostring() << std::endl;
        } else {
            std::cout << "unknown" << std::endl;
        }
        return 0;
    }

    app::app() : m_loop(uv_default_loop()) {
        *static_cast<app**>(lua_getextraspace(m_lua.native())) = this;
        uv_loop_set_data(m_loop.native(),this);
        m_lua.open_libs();
        lua_atpanic(lua().native(),&at_panic);
        lua::bind::object<meta::object>::register_metatable(lua());
    }

    app::~app() {
        //m_loop.stop();
        //m_lua.close();
    }

    app& app::get(lua_State* L) {
        return **static_cast<app**>(lua_getextraspace(L));
    }

    app& app::get(uv_loop_t* l) {
        return *static_cast<app*>(uv_loop_get_data(l));
    }

    void app::run() {
        int v = m_loop.run(UV_RUN_DEFAULT);
        if (v != 0) {
            /* todo */
        }
        m_lua.close();
    }

    void app::show_error(lua::state& l,lua::status e) {
        switch(e) {
            case lua::status::yield:
                std::cout << "YIELD:\t";
                break;
            case lua::status::errun:
                std::cout << "ERRRUN:\t";
                break;
            case lua::status::errsyntax:
                std::cout << "ERRSYNTAX:\t";
                break;
            case lua::status::errmem:
                std::cout << "ERRMEM:\t";
                break;
            case lua::status::errgcmm:
                std::cout << "ERRGCMM:\t";
                break;
            case lua::status::errerr:
                std::cout << "ERRERR:\t";
                break;
            default:
                std::cout << "UNKNOWN:\t";
                break;
        };
        lua::value err(l,-1);
        if (err.is_string()) {
            std::cout << err.tostring() << std::endl;
        } else {
            std::cout << "unknown" << std::endl;
        }
    }

    
}


int luaopen_llae(lua_State* L) {
    lua::state s(L);
    luaL_Reg reg[] = {
        //{ "run", lua_llae_run },
        /* { "set_handler", lua_llae_set_handler }, */
        // { "sleep", &Timer::sleep },
        // 
        // { "dump", lua_llae_dump },
        // { "get_exepath", lua_llae_get_exepath },
        // { "get_cwd", lua_llae_get_cwd },
        // { "get_home", lua_llae_get_home },
        // { "newTCPServer", &TCPServer::lnew },
        // { "newTCPConnection", &TCPConnection::lnew },
        // { "newTimer", &Timer::lnew },
        // { "newIdle", &Idle::lnew },
        // { "getaddrinfo", &GetAddrInfoLuaThreadReq::lua_getaddrinfo },
        // { "newTLSCtx", &TLSCtx::lnew },
        // { "newTLS",&TLS::lnew },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}

/*
#include "tcp_server.h"
#include "tcp_connection.h"
#include "timer.h"
#include "file.h"
#include "idle.h"
#include "dns.h"
#include "tls.h"

#include <iostream>
#include <signal.h>

#if defined(UV_VERSION_HEX) && (UV_VERSION_HEX >= 0x011300) 
#else
static int uv_handle_type_name(int t) { return t; }
static void* uv_loop_set_data(uv_loop_t* loop, void* data) {
    loop->data = data;
    return data;
}
#endif



static void dump_walk_cb(uv_handle_t* handle,void* arg) {
    std::cout << "handle: " << uv_handle_type_name(handle->type) << "\t" << uv_is_closing(handle) << "\t" << uv_is_active(handle) << std::endl;
}

static int lua_llae_dump(lua_State* L) {
    uv_walk(llae_get_loop(L),dump_walk_cb,0);
    std::cout << "num uv objects: " << RefCounter::get_num_objects() << std::endl;
    return 0;
}

static uv_loop_t* action_loop = 0;
static uv_async_t stop_async;
static void stop_cb(uv_async_t* ) {
    uv_stop(stop_async.loop);
    uv_close(reinterpret_cast<uv_handle_t*>(&stop_async),0);
}




static int lua_llae_get_exepath(lua_State* L) {
    size_t size = LUAL_BUFFERSIZE;
    luaL_Buffer buf;
    char* data = luaL_buffinitsize(L,&buf,size);
    int r = uv_exepath(data,&size);
    lua_llae_handle_error(L,"get_exepath",r);
    luaL_pushresultsize(&buf,size);
    return 1;
}

static int lua_llae_get_cwd(lua_State* L) {
    size_t size = LUAL_BUFFERSIZE;
    luaL_Buffer buf;
    char* data = luaL_buffinitsize(L,&buf,size);
    int r = uv_cwd(data,&size);
    lua_llae_handle_error(L,"get_cwd",r);
    luaL_pushresultsize(&buf,size);
    return 1;
}

static int lua_llae_get_home(lua_State* L) {
    size_t size = LUAL_BUFFERSIZE;
    luaL_Buffer buf;
    char* data = luaL_buffinitsize(L,&buf,size);
    int r = uv_os_homedir(data,&size);
    lua_llae_handle_error(L,"get_home",r);
    luaL_pushresultsize(&buf,size);
    return 1;
}


extern "C" int luaopen_llae(lua_State* L) {
    *static_cast<uv_loop_t**>(lua_getextraspace(L)) = uv_default_loop();
    uv_loop_set_data(uv_default_loop(),L);

    Stream::lbind(L);
	TCPServer::lbind(L);
    TCPConnection::lbind(L);
    Timer::lbind(L);
    File::lbind(L);
    Idle::lbind(L);
    TLSCtx::lbind(L);
    TLS::lbind(L);
	luaL_Reg reg[] = {
        { "run", lua_llae_run },
        { "sleep", &Timer::sleep },
        { "set_handler", lua_llae_set_handler },
        { "dump", lua_llae_dump },
        { "get_exepath", lua_llae_get_exepath },
        { "get_cwd", lua_llae_get_cwd },
        { "get_home", lua_llae_get_home },
        { "newTCPServer", &TCPServer::lnew },
        { "newTCPConnection", &TCPConnection::lnew },
        { "newTimer", &Timer::lnew },
        { "newIdle", &Idle::lnew },
        { "getaddrinfo", &GetAddrInfoLuaThreadReq::lua_getaddrinfo },
        { "newTLSCtx", &TLSCtx::lnew },
        { "newTLS",&TLS::lnew },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}
*/