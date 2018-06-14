#include "llae.h"
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

static int lua_llae_run(lua_State* L) {
	uv_loop_t* loop = llae_get_loop(L);
	int v = uv_run(loop,UV_RUN_DEFAULT);
    if (v != 0) {
        lua_gc(L,LUA_GCCOLLECT,0);
        uv_run(loop,UV_RUN_NOWAIT);
        uv_run(loop,UV_RUN_NOWAIT);
    }
	return 0;
}

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

static void uv_stop_laction (int i) {
  signal(i, SIG_DFL); 
  if (action_loop) {
    uv_async_init(action_loop,&stop_async,&stop_cb);
    uv_async_send(&stop_async);
    action_loop = 0;
  }
}

static int lua_llae_set_handler(lua_State* L) {
    action_loop = llae_get_loop(L);
    signal(SIGINT, uv_stop_laction);
    return 0;
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
        { "newThread", &Idle::lnew },
        { "getaddrinfo", &GetAddrInfoLuaThreadReq::lua_getaddrinfo },
        { "newTLSCtx", &TLSCtx::lnew },
        { "newTLS",&TLS::lnew },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    lua_newtable(L);
    luaL_Reg freg[] = {
        { "stat", File::stat },
        { "open", File::open },
        { "mkdir", File::mkdir },
        { "scandir", File::scandir },
        { NULL, NULL }
    };
    luaL_setfuncs(L, freg, 0);
    lua_setfield(L,-2,"file");
    return 1;
}