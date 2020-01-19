#include "idle.h"
#include "luabind.h"
#include <new>
#include <cassert>

static const char* mt_name = "llae.Idle";

Idle::Idle(uv_loop_t* loop) : m_need_stop(false) {
	uv_idle_init(loop,&m_idle);
	attach();
}

int Idle::lnew(lua_State* L) {
	(new Idle(llae_get_loop(L)))->push(L);
	return 1;
}

void Idle::push(lua_State* L) {
	new (lua_newuserdata(L,sizeof(IdleRef))) 
			IdleRef(this);
	UVHandleHolder::push(L);
	luaL_setmetatable(L,mt_name);
}

uv_handle_t* Idle::get_handle() {
	return reinterpret_cast<uv_handle_t*>(&m_idle);
}

void Idle::on_gc(lua_State* L) {
	m_th.reset(L);
	uv_idle_stop(&m_idle);
}

void Idle::on_idle() {
	lua_State* L = llae_get_vm(get_handle());
	if (L && m_th) {
		if (!m_th.resumev(L,"Idle::on_idle")) {
			stop(L);
		}
	}
}

void Idle::idle_cb(uv_idle_t* idle) {
	static_cast<Idle*>(idle->data)->on_idle();
}

void Idle::start(lua_State* L,const luabind::function& f) {
	m_th.start(L,f,"Idle::start",IdleRef(this));
	int res = uv_idle_start(&m_idle,&Idle::idle_cb);
	if (res < 0) {
		m_th.reset(L);
		lua_llae_handle_error(L,"Idle::start",res);
	} else {
		m_need_stop = true;
	}
}
void Idle::stop(lua_State* L) {
	m_th.reset(L);

	if (m_need_stop) {
		int res = uv_idle_stop(&m_idle);
		m_need_stop = false;
		lua_llae_handle_error(L,"Idle::stop",res);
	}
	
}

void Idle::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"start",&Idle::start);
	luabind::bind(L,"stop",&Idle::stop);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&UVHandleHolder::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}
