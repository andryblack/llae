#ifndef _LLAE_LUA_STATE_H_INCLUDED_
#define _LLAE_LUA_STATE_H_INCLUDED_

#include <cstring>
#include "lua/types.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


namespace lua {

	class state {
	protected:
		lua_State* m_L;
	public:
		explicit state(lua_State* L) : m_L(L) {}
		state(const state& st) : m_L(st.m_L) {}
		state( state&& st) : m_L(st.m_L) { st.m_L = nullptr; }
		state& operator = (const state& st) { m_L = st.m_L; return *this; }
		bool operator == (const state& s) const { return m_L == s.m_L; }
		bool operator != (const state& s) const { return m_L != s.m_L; }
		lua_State* native() { return m_L; }
		value_type get_type(int idx) const { 
			return static_cast<value_type>(lua_type(m_L,idx)); 
		}
		int gettop() { return lua_gettop(m_L); }
		void createtable(int narr=0,int nset=0) { lua_createtable(m_L,narr,nset); }
		void pushinteger(lua_Integer val) { lua_pushinteger(m_L,val); }
		void pushstring(const char* str) { lua_pushstring(m_L,str); }
		void pushcclosure(lua_CFunction func,int nup) { lua_pushcclosure(m_L,func,nup); }
		void rawseti(int tidx,int idx) { lua_rawseti(m_L,tidx,idx); }
		void setglobal(const char* name) { lua_setglobal(m_L,name); }
		value_type getglobal(const char* name) { return static_cast<value_type>(lua_getglobal(m_L,name)); }
		value_type getfield(int tidx,const char* name) { return static_cast<value_type>(lua_getfield(m_L,tidx,name)); }
		void remove(int idx) { lua_remove(m_L,idx); }
		bool toboolean(int idx) const { return lua_toboolean(m_L,idx); }
		lua_Number tonumber(int idx) const { return lua_tonumber(m_L,idx); }
		lua_Integer tointeger(int idx) const { return lua_tointeger(m_L,idx); }
		const char* tostring(int idx) const { return lua_tostring(m_L,idx); }
		status dostring(const char* str);
		void call(int nargs,int nresult) { lua_call(m_L,nargs,nresult); }
		void require(const char* modname,lua_CFunction func);
		void pushvalue(int idx) const { lua_pushvalue(m_L,idx); }
		int ref() { return luaL_ref(m_L,LUA_REGISTRYINDEX); }
		void unref(int r) { luaL_unref(m_L,LUA_REGISTRYINDEX,r); }
		void pushref(int r) { lua_geti(m_L,LUA_REGISTRYINDEX,r); }
		void xmove(const state& s,int vals=1) const {
			lua_xmove(s.m_L,m_L,vals);
		}
		void error() { lua_error(m_L); }
		template<class ... Types>
		void error(const char *fmt, Types ... args) { luaL_error(m_L,fmt,args...); }

	};

	class main_state : public state {
	private:
		main_state(const main_state&) = delete;
		main_state& operator = (const state&) = delete;
		static void *lua_alloc (void *ud, void *ptr, size_t osize,
                                                size_t nsize);
	public:
		main_state();
		~main_state();
		void close();
		void open_libs();
	};
}

#endif /*_LUA_STATE_H_INCLUDED_*/