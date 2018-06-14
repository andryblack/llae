#ifndef __LLAE_LLAE_H_INCLUDED__
#define __LLAE_LLAE_H_INCLUDED__

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <uv.h>

static inline uv_loop_t* llae_get_loop(lua_State* L) {
	return *static_cast<uv_loop_t**>(lua_getextraspace(L));
}
static inline void lua_llae_handle_error(lua_State* L,const char* name,int res) {
	if (res < 0) {
		luaL_error(L,"error %s on %s",uv_strerror(res),name);
	}
}
template <class T>
static inline lua_State* llae_get_vm(T* handle) {
	if (handle && handle->loop)
		return static_cast<lua_State*>(handle->loop->data);
	return 0;
}
static inline lua_State* llae_get_vm(uv_loop_t* loop) {
	if (loop)
		return static_cast<lua_State*>(loop->data);
	return 0;
}

extern "C" int luaopen_llae(lua_State* L);


#endif /*__LLAE_LLAE_H_INCLUDED__*/