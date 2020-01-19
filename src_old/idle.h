#ifndef __LLAE_IDLE_H_INCLUDED__
#define __LLAE_IDLE_H_INCLUDED__


#include "llae.h"
#include "uv_handle_holder.h"
#include "lua_holder.h"

class Idle : public UVHandleHolder {
private:
	uv_idle_t m_idle;
protected:
	virtual uv_handle_t* get_handle();
	LuaThread m_th;
	bool m_need_stop;
	void on_idle();
	void on_gc(lua_State* L);
private:	
	static void idle_cb(uv_idle_t* handle);
public:
	explicit Idle(uv_loop_t* loop);
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);
	void push(lua_State* L);
	void start(lua_State* L,const luabind::function& f);
	void stop(lua_State* L);
};
typedef Ref<Idle> IdleRef;


#endif /*__LLAE_IDLE_H_INCLUDED__*/