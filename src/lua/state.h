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
		void* newuserdata(size_t size) { return lua_newuserdata(m_L,size); }
		void createtable(int narr=0,int nset=0) { lua_createtable(m_L,narr,nset); }
		void newtable() { lua_newtable(m_L); }
		void pushboolean(bool v) { lua_pushboolean(m_L,v?1:0);}
		void pushinteger(lua_Integer val) { lua_pushinteger(m_L,val); }
		void pushnumber(lua_Number val) { lua_pushnumber(m_L,val); }
		void pushstring(const char* str) { lua_pushstring(m_L,str); }
		void pushlstring(const char* str,size_t len) { lua_pushlstring(m_L,str,len); }
		template<class ... Types>
		void pushfstring(const char *fmt, Types ... args) { lua_pushfstring(m_L,fmt,args...); }
		void pushcclosure(lua_CFunction func,int nup) { lua_pushcclosure(m_L,func,nup); }
		void pushcfunction(lua_CFunction func) { lua_pushcfunction(m_L,func); }
		void pushlightuserdata(void *p) { lua_pushlightuserdata(m_L,p); }
		void pushnil() { lua_pushnil(m_L); }
		void pushthread() { lua_pushthread(m_L); }
		void rawseti(int tidx,int idx) { lua_rawseti(m_L,tidx,idx); }
		void rawgeti(int tidx,int idx) { lua_rawgeti(m_L,tidx,idx); }
		size_t len(int idx) { return luaL_len(m_L,idx); }
		size_t rawlen(int idx) { return lua_rawlen(m_L,idx); }
		void setfield(int index, const char *k) { lua_setfield(m_L,index,k); }
		void seti(int index, lua_Integer n) { lua_seti(m_L,index,n); }
		void geti(int index, lua_Integer n) { lua_geti(m_L,index,n); }
		void setglobal(const char* name) { lua_setglobal(m_L,name); }
		value_type getglobal(const char* name) { return static_cast<value_type>(lua_getglobal(m_L,name)); }
		value_type getfield(int tidx,const char* name) { return static_cast<value_type>(lua_getfield(m_L,tidx,name)); }
		void remove(int idx) { lua_remove(m_L,idx); }
		bool toboolean(int idx) const { return lua_toboolean(m_L,idx); }
		lua_Number tonumber(int idx) const { return lua_tonumber(m_L,idx); }
		lua_Number optnumber(int idx,lua_Number def) const { return luaL_optnumber(m_L,idx,def); }
		lua_Number checknumber(int idx) const { return luaL_checknumber(m_L,idx); }
		lua_Integer tointeger(int idx) const { return lua_tointeger(m_L,idx); }
		lua_Integer optinteger(int idx,lua_Integer d) const { return luaL_optinteger(m_L,idx,d); }
		lua_Integer checkinteger(int idx) const { return luaL_checkinteger(m_L,idx); }
		const char* tostring(int idx) const { return lua_tostring(m_L,idx); }
		const char* checkstring(int idx) const { return luaL_checkstring(m_L,idx); }
		const char* optstring(int idx,const char* d) const { return luaL_optstring(m_L,idx,d); }
		const char* checklstring(int idx,size_t& size) { return luaL_checklstring(m_L,idx,&size);}
		const char* tolstring(int idx,size_t& size) { return lua_tolstring(m_L,idx,&size); }
		void* touserdata(int idx) const { return lua_touserdata(m_L,idx); }

		void checktype(int arg, value_type t) { luaL_checktype(m_L,arg,static_cast<int>(t)); }
		status dostring(const char* str);
		void call(int nargs,int nresult) { lua_call(m_L,nargs,nresult); }
		status pcall(int nargs,int nresult, int msgh) { return static_cast<status>(lua_pcall(m_L,nargs,nresult,msgh)); }
		void require(const char* modname,lua_CFunction func);
		void pushvalue(int idx) const { lua_pushvalue(m_L,idx); }
		int ref() { return luaL_ref(m_L,LUA_REGISTRYINDEX); }
		void unref(int r) { luaL_unref(m_L,LUA_REGISTRYINDEX,r); }
		void pushref(int r) { lua_geti(m_L,LUA_REGISTRYINDEX,r); }
		void pop(int cnt) { lua_pop(m_L,cnt); }
		void xmove(const state& s,int vals=1) const {
			lua_xmove(s.m_L,m_L,vals);
		}
		void error() { lua_error(m_L); }
		template<class ... Types>
		void error(const char *fmt, Types ... args) { luaL_error(m_L,fmt,args...); }
		void argerror(int arg, const char *extramsg) { luaL_argerror(m_L,arg,extramsg); }
		bool isyieldable() const { return lua_isyieldable(m_L); }
		void yield(int nresult) {
			lua_yield(m_L,nresult);
		}
		status resume(state& from,int nargs) {
			return static_cast<status>(lua_resume(m_L,from.native(),nargs));
		}
		state tothread(int idx) const {
			return state(lua_tothread(m_L,idx));
		}
		bool newmetatable(const char* name) { return luaL_newmetatable(m_L,name); }
		value_type getmetatable(const char* name) { return static_cast<value_type>(luaL_getmetatable(m_L,name));}
		void setmetatable(int idx) { lua_setmetatable(m_L,idx); }
		void checkstack(int size) {lua_checkstack(m_L,size); }
        char* buffinitsize(luaL_Buffer& b,size_t sz) { return luaL_buffinitsize(m_L,&b,sz); }
        void pushresultsize(luaL_Buffer& b,size_t sz) { luaL_pushresultsize(&b,sz); }
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
