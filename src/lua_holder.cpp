#include "lua_holder.h"
#include <cassert>
#include <iostream>

LuaHolder::LuaHolder() : m_ref(LUA_NOREF) {

}

LuaHolder::~LuaHolder() {
	assert(m_ref == LUA_NOREF);
}

void LuaHolder::assign(lua_State* L) {
	if (m_ref != LUA_NOREF) {
		reset(L);
	}
	m_ref = luaL_ref(L,LUA_REGISTRYINDEX);
}

void LuaHolder::assign_check(lua_State* L,int t) {
	if (m_ref != LUA_NOREF) {
		reset(L);
	}
	if (lua_type(L,-1)!=t) {
		luaL_error(L,"%s expected",lua_typename(L, t));
	}
	m_ref = luaL_ref(L,LUA_REGISTRYINDEX);
}

void LuaHolder::reset(lua_State* L) {
	luaL_unref(L,LUA_REGISTRYINDEX,m_ref);
	m_ref = LUA_NOREF;
}

void LuaHolder::push(lua_State* L) {
	lua_geti(L,LUA_REGISTRYINDEX,m_ref);
}

void LuaFunction::assign(lua_State* L,const luabind::function& f) {
	f.push(L);
	LuaHolder::assign_check(L,LUA_TFUNCTION);
}

void lua_check_result(lua_State* L,int r,const char* func) {
	if (r!=LUA_OK) {
		const char* errmsg = lua_tostring(L,-1);
		if (errmsg) {
			std::cerr << "failed call " << func << ": " << errmsg << std::endl;
			lua_pop(L,1);
		} else {
			std::cerr << "failed call " << func << " " << r << std::endl;
		}
	}
}

int lua_error_handler(lua_State* L) {
	const char* err = lua_tostring(L,1);
	luaL_traceback(L,L,err,0);
	return 1;
}

void lua_check_resume_result(lua_State* L,int r,const char* func) {
	if (r!=LUA_OK) {
		if (r == LUA_ERRMEM) {
			std::cerr << "failed resume " << func << ": LUA_ERRMEM";
		} else if (r == LUA_ERRERR) {
			std::cerr << "failed resume " << func << ": LUA_ERRERR";
		} else if (r == LUA_ERRGCMM) {
			std::cerr << "failed resume " << func << ": LUA_ERRGCMM";
		} else {
			const char* errmsg = lua_tostring(L,-1);
			if (errmsg) {
				std::cerr << "failed resume " << func << ": " << errmsg << std::endl;
				lua_pop(L,1);
			} else {
				std::cerr << "failed resume code " << func << " " << r << std::endl;
			}
		}
	}
}

void LuaThread::assign(lua_State* L,const luabind::thread& t) {
	t.push(L);
	LuaHolder::assign_check(L,LUA_TTHREAD);
}

void LuaThread::starti(lua_State* L, const luabind::function& f, const char* func, int numarg) {
	reset(L);
	//std::cout << "LuaThread::starti: " << func << ":" << lua_gettop(L) << "/" << numarg << std::endl;
	int n = lua_gettop(L)-numarg+1;
	f.push(L);
	if (numarg) {
		lua_insert(L,n);
	}
	lua_State* t = lua_newthread(L);
	lua_insert(L,n);
	lua_xmove(L,t,1+numarg);
	t = lua_tothread(L,n);
	assert(t);

	assert(lua_isfunction(t,-1-numarg));
	int r = lua_resume(t,L,numarg);

	//printf("lua_resume %d %d\n",n-2,r);
	if (r == LUA_YIELD) {
		assign_check(L,LUA_TTHREAD);
	} else if (r == LUA_OK) {
		lua_pop(L,1);
		luaL_error(L,"coroutine end without yield on %s",func);
	} else {
		char buf[128];
		snprintf(buf,128,"failed coroutine start of %s",func);
		luaL_traceback(L,t,buf,0);
		lua_error(L);
	}
}