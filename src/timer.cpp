#include "timer.h"
#include "luabind.h"
#include <new>

static const char* mt_name = "llae.Timer";


TimerBase::TimerBase(uv_loop_t* loop) {
	uv_timer_init(loop,&m_timer);
	attach();
}

uv_handle_t* TimerBase::get_handle() {
	return reinterpret_cast<uv_handle_t*>(&m_timer);
}

Timer::Timer(uv_loop_t* loop) : TimerBase(loop) {
}

int Timer::lnew(lua_State* L) {
	(new Timer(llae_get_loop(L)))->push(L);
	return 1;
}


void Timer::push(lua_State* L) {
	new (lua_newuserdata(L,sizeof(TimerRef))) 
			TimerRef(this);
	UVHandleHolder::push(L);
	luaL_setmetatable(L,mt_name);
}

void Timer::on_gc(lua_State* L) {
	m_cb.reset(L);
	uv_timer_stop(&m_timer);
}

void Timer::on_timer() {
	lua_State* L = llae_get_vm(get_handle());
	if (L && m_cb) {
		m_cb.callv(L,"Timer::on_timer");
	}
}

void Timer::timer_cb(uv_timer_t* timer) {
	static_cast<Timer*>(timer->data)->on_timer();
}

void Timer::start(lua_State* L,const luabind::function& f,uint64_t timeout,uint64_t repeat) {
	m_cb.assign(L,f);
	int res = uv_timer_start(&m_timer,&timer_cb,timeout,repeat);
	lua_llae_handle_error(L,"Timer::start",res);
}
void Timer::stop(lua_State* L) {
	int res = uv_timer_stop(&m_timer);
	m_cb.reset(L);
	lua_llae_handle_error(L,"Timer::stop",res);
}

ThreadTimer::ThreadTimer(uv_loop_t* loop) : TimerBase(loop) {
}

void ThreadTimer::start(lua_State* L,uint64_t timeout) {
	lua_pushthread(L);
	m_th.assign(L);
	int res = uv_timer_start(&m_timer,&timer_cb,timeout,0);
	lua_llae_handle_error(L,"ThreadTimer::start",res);
	add_ref();
}

void ThreadTimer::timer_cb(uv_timer_t* timer) {
	ThreadTimer* tmr = static_cast<ThreadTimer*>(timer->data);
	tmr->on_timer();
	tmr->remove_ref();
}
void ThreadTimer::on_timer() {
	lua_State* L = llae_get_vm(m_timer.loop);
	if (L) {
		if (m_th) {
			m_th.resumev(L,"ThreadTimer::on_timer");
			m_th.reset(L);
		}
	}
}

int Timer::sleep(lua_State* L) {
	if (!lua_isyieldable(L)) {
		luaL_error(L,"Timer::sleep called not from thread");
	}
	{
		lua_Integer delay = luaL_checkinteger(L,1);
		ThreadTimerRef ref(new ThreadTimer(llae_get_loop(L)));
		ref->start(L,delay);
	}
	return lua_yield(L,1);
}

void Timer::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"start",&Timer::start);
	luabind::bind(L,"stop",&Timer::stop);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&UVHandleHolder::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}
