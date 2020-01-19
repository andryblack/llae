#ifndef __LLAE_LUA_REF_H_INCLUDED__
#define __LLAE_LUA_REF_H_INCLUDED__

#include "lua_value.h"

namespace lua {

	class ref {
	private:
		int m_ref;
	public:
		ref() : m_ref(LUA_NOREF) {}
		~ref() { assert(m_ref == LUA_NOREF); }
		ref(const ref&& l) : m_ref(l.m_ref) { l.m_ref = LUA_NOREF; }
		void assign( value& val) {
			state_ref& s(val.state());
			reset(s);
			val.push();
			m_ref = s.ref();
		}
		value push(state_ref& s) {
			s.pushref();
			return value(s,-1);
		}
		void reset(state_ref& s) {
			if (m_ref != LUA_NOREF) {
				s.unref(m_ref);
				m_ref = LUA_NOREF;
			}
		}

		typedef int LuaHolder::*unspecified_bool_type;
	        
	    operator unspecified_bool_type() const {
	        return (m_ref != LUA_NOREF) ? &LuaHolder::m_ref : 0;
	    }
	};

}
/*

void lua_check_result(lua_State* L,int r,const char* func);
void lua_check_resume_result(lua_State* L,int r,const char* func);
int lua_error_handler(lua_State* L);

class LuaFunction : public LuaHolder {
protected:
	
public:
	void assign(lua_State* L,const luabind::function& f);
	void callv(lua_State* L,const char* func) {
		lua_pushcfunction(L,lua_error_handler);
		push(L);
		int res = lua_pcall(L,0,0,-2);
		lua_check_result(L,res,func);
		lua_pop(L,1);
	}
	template <typename ...Args>
	void callv(lua_State* L,const char* func,Args ... args) {
		lua_pushcfunction(L,lua_error_handler);
		push(L);
		luabind::push(L,args...);
		int res = lua_pcall(L, sizeof...(args),0,-3);
		lua_check_result(L,res,func);
		lua_pop(L,1);
	}
};

class LuaThread : public LuaHolder {
public:
	void starti(lua_State* L, const luabind::function& f, const char* func,int argc);
	template <class A0>
	void start(lua_State* L, const luabind::function& f, const char* func,A0 a0) {
		luabind::S<A0>::push(L,a0);
		starti(L,f,func,1);
	}
	void assign(lua_State* L) { LuaHolder::assign_check(L,LUA_TTHREAD); }
	void assign(lua_State* L,const luabind::thread& f);
	template <typename ...Args>
	bool resumev(lua_State* L,const char* func,Args ... args) {
		push(L);
		lua_State* t = lua_tothread(L,-1);
		int dummy[] = {(luabind::S<Args>::push(L,args),0)...};
		lua_xmove(L,t,sizeof...(Args));
		int r = lua_resume(t,L,sizeof...(Args));
		if (r != LUA_YIELD) {
			lua_check_resume_result(t,r,func);
		}
		lua_pop(L,1);
		return r == LUA_YIELD;
	}
	bool resumevi(lua_State* L,const char* func,int na) {
		push(L);
		lua_State* t = lua_tothread(L,-1);
		lua_pop(L,1);
		//int tt = lua_gettop(t);
		lua_xmove(L,t,na);
		int r = lua_resume(t,L,na);
		if (r != LUA_YIELD ) {
			lua_check_resume_result(t,r,func);
		}
		return r == LUA_YIELD;
	}
};
*/

#endif /*__LLAE_LUA_REF_H_INCLUDED__*/
