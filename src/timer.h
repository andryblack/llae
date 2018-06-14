#ifndef __LLAE_TIMER_H_INCLUDED__
#define __LLAE_TIMER_H_INCLUDED__


#include "llae.h"
#include "uv_handle_holder.h"
#include "lua_holder.h"

class TimerBase : public UVHandleHolder {
protected:
	uv_timer_t m_timer;
	virtual uv_handle_t* get_handle();
	explicit TimerBase(uv_loop_t* loop);
};

class Timer : public TimerBase {
protected:
	LuaFunction m_cb;

	void on_timer();
	void on_gc(lua_State* L);
private:	
	static void timer_cb(uv_timer_t* handle);
public:
	explicit Timer(uv_loop_t* loop);
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);
	void push(lua_State* L);
	void start(lua_State* L,const luabind::function& f,uint64_t timeout,uint64_t repeat);
	void stop(lua_State* L);
	static int sleep(lua_State* L);
};
typedef Ref<Timer> TimerRef;

class ThreadTimer : public TimerBase {
private:
	LuaThread m_th;
	void on_timer();
	static void timer_cb(uv_timer_t* handle);
public:
	explicit ThreadTimer(uv_loop_t* loop);
	void start(lua_State* L,uint64_t timeout);
};
typedef Ref<ThreadTimer> ThreadTimerRef;

#endif /*__LLAE_TIMER_H_INCLUDED__*/