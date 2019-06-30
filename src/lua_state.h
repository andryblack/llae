#ifndef _LUA_STATE_H_INCLUDED_
#define _LUA_STATE_H_INCLUDED_

#include <cstring>

#include "lua_types.h"

namespace Lua {

	class StateRef {
	protected:
		lua_State* m_L;
	public:
		StateRef(lua_State* L) : m_L(L) {}
		StateRef(const StateRef& st) : m_L(st.m_L) {}
		StateRef( StateRef&& st) : m_L(st.m_L) { st.m_L = nullptr; }
		StateRef& operator = (const StateRef& st) { m_L = st.m_L; return *this; }

		ValueType get_type(int idx) const { 
			return static_cast<ValueType>(lua_type(m_L,idx)); 
		}
		void createtable(int narr,int nset) { lua_createtable(m_L,narr,nset); }
		void pushinteger(lua_Integer val) { lua_pushinteger(m_L,val); }
		void pushstring(const char* str) { lua_pushstring(m_L,str); }
		void pushcclosure(lua_CFunction func,int nup) { lua_pushcclosure(m_L,func,nup); }
		void rawseti(int tidx,int idx) { lua_rawseti(m_L,tidx,idx); }
		void setglobal(const char* name) { lua_setglobal(m_L,name); }
		ValueType getglobal(const char* name) { return static_cast<ValueType>(lua_getglobal(m_L,name)); }
		ValueType getfield(int tidx,const char* name) { return static_cast<ValueType>(lua_getfield(m_L,tidx,name)); }
		void remove(int idx) { lua_remove(m_L,idx); }
		bool toboolean(int idx) const { return lua_toboolean(m_L,idx); }
		lua_Number tonumber(int idx) const { return lua_tonumber(m_L,idx); }
		lua_Integer tointeger(int idx) const { return lua_tointeger(m_L,idx); }
		const char* tostring(int idx) const { return lua_tostring(m_L,idx); }
		Status dostring(const char* str);
		void call(int nargs,int nresult) { lua_call(m_L,nargs,nresult); }
		void require(const char* modname,lua_CFunction func);
		int ref();
		void unref(int r);
	};

	class State : public StateRef {
	private:
		State(const State&) = delete;
		State& operator = (const State&) = delete;
		static void *lua_alloc (void *ud, void *ptr, size_t osize,
                                                size_t nsize);
	public:
		State();
		~State();
		void close();
		void open_libs();
	};
}

#endif /*_LUA_STATE_H_INCLUDED_*/