#ifndef __LLAE_LUABIND_H_INCLUDED__
#define __LLAE_LUABIND_H_INCLUDED__

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <cstdint>
#include "ref_counter.h"

#include  <utility>

namespace luabind {

	static void check_type(lua_State* L,int idx,int type) {
		int at = lua_type(L,idx);
		if (at != type) {
			luaL_error(L,"invalid value %s at %d expected %s",lua_typename(L,at),idx,lua_typename(L,type));
		}
	}
	class lnil {
	};
	class function {
	private:
		int m_idx;
	public:
		explicit function(int idx) : m_idx(idx) {}
		function(const function& f) : m_idx(f.m_idx) {}
		void push(lua_State* L) const { 
			check_type(L,m_idx,LUA_TFUNCTION);
			lua_pushvalue(L,m_idx); 
		}
	};
	class thread {
	private:
		int m_idx;
	public:
		explicit thread(int idx) : m_idx(idx) { }
		thread(const thread& f) : m_idx(f.m_idx) { }
		void push(lua_State* L) const { 
			check_type(L,m_idx,LUA_TTHREAD);
			lua_pushvalue(L,m_idx); 
		}
	};
	class table {
	private:
		int m_idx;
	public:
		explicit table(int idx) : m_idx(idx) { }
		void push(lua_State* L) const { 
			check_type(L,m_idx,LUA_TTABLE);
			lua_pushvalue(L,m_idx); 
		}
	};
	template <class T>
	struct S;
	template <>
	struct S<int> {
		static int get(lua_State* L,int idx) {
			return lua_tointeger(L,idx);
		}
	};
	template <>
	struct S<bool> {
		static bool get(lua_State* L,int idx) {
			return lua_toboolean(L,idx);
		}
		static void push(lua_State* L,bool r) {
			lua_pushboolean(L,r?1:0);
		}
	};
	template <>
	struct S<uint64_t> {
		static uint64_t get(lua_State* L,int idx) {
			return lua_tointeger(L,idx);
		}
	};
	template <>
	struct S<lua_Integer> {
		static lua_Integer get(lua_State* L,int idx) {
			return lua_tointeger(L,idx);
		}
		static void push(lua_State* L,lua_Integer v) {
			lua_pushinteger(L,v);
		}
	};
	template <>
	struct S<lua_Number> {
		static lua_Number get(lua_State* L,int idx) {
			return lua_tonumber(L,idx);
		}
		static void push(lua_State* L,lua_Number v) {
			lua_pushnumber(L,v);
		}
	};
	template <>
	struct S<const char*> {
		static const char* get(lua_State* L,int idx) {
			return lua_tostring(L,idx);
		}
		static void push(lua_State* L,const char* v) {
			if (v) lua_pushstring(L,v);
			else lua_pushnil(L);
		}
	};
	template <>
	struct S<const function&> {
		static function get(lua_State* L,int idx) {
			luaL_checktype(L,idx,LUA_TFUNCTION);
			return function(idx);
		}
	};
	template <>
	struct S<function> : S<const function&> {};

	template <>
	struct S<const thread&> {
		static thread get(lua_State* L,int idx) {
			luaL_checktype(L,idx,LUA_TTHREAD);
			return thread(idx);
		}
	};
	template <>
	struct S<thread> : S<const thread&> {};
	
	template <>
	struct S<const table&> {
		static void push(lua_State* L,const table& v) {
			v.push(L);
		}
	};
	template <>
	struct S<table> : S<const table&> {};

	template <>
	struct S<const lnil&> {
		static void push(lua_State* L,const lnil& v) {
			lua_pushnil(L);
		}
	};
	template <>
	struct S<lnil> : S<const lnil&> {};

	
	template <class T>
	struct S<const Ref<T>&> {
		static void push(lua_State* L,const Ref<T>& h) {
			h->push(L);
		}
		static Ref<T> get(lua_State* L,int idx) {
			return Ref<T>::get_ref(L,idx);
		}
	};
	template <class T>
	struct S<Ref<T> > :  S<const Ref<T>&> {};

	template <class T,class R, typename ...Args>
	struct functions_impl{
		template <int ...Is> struct expand_t {
			typedef R (T::*LFunc)(lua_State*,Args...);
			typedef R (T::*Func)(Args...);
		
			static int lf(lua_State* L) {
				LFunc func = *reinterpret_cast<LFunc*>(lua_touserdata(L, lua_upvalueindex(1)));
				T* ref = Ref<T>::get_ptr(L,1);
				S<R>::push(L,(ref->*func)(L,S<Args>::get(L,2+Is)...));
				return 1;
			} 
			static int f(lua_State* L) {
				Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
				T* ref = Ref<T>::get_ptr(L,1);
				S<R>::push(L,(ref->*func)(S<Args>::get(L,2+Is)...));
				return 1;
			} 
		};
		template <int ...Is>
		static expand_t<Is...> expand(std::integer_sequence<int,Is...>) {
			return expand_t<Is...>();
		}
	};
	template <class T,typename ...Args>
	struct functions_impl<T,void,Args...> {
		template <int ...Is> struct expand_t {
			typedef void (T::*LFunc)(lua_State*,Args...);
			typedef void (T::*Func)(Args...);
		
			static int lf(lua_State* L) {
				LFunc func = *reinterpret_cast<LFunc*>(lua_touserdata(L, lua_upvalueindex(1)));
				T* ref = Ref<T>::get_ptr(L,1);
				(ref->*func)(L,S<Args>::get(L,2+Is)...);
				return 0;
			} 
			static int f(lua_State* L) {
				Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
				T* ref = Ref<T>::get_ptr(L,1);
				(ref->*func)(S<Args>::get(L,2+Is)...);
				return 0;
			} 
		};
		template <int ...Is>
		static expand_t<Is...> expand(std::integer_sequence<int,Is...>) {
			return expand_t<Is...>();
		}
	};
	template <class T,class R,typename ...Args>
	using functions = decltype(functions_impl<T,R,Args...>::expand(std::make_integer_sequence<int,sizeof...(Args)>()));

	template <class T,class R,typename ...Args>
	static inline void bind(lua_State* L, const char* name, R(T::*func)(lua_State* L,Args...)) {
		typedef functions<T,R,Args...> functions_t;
		typedef typename functions_t::LFunc Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&functions_t::lf,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class R,typename ...Args>
	static inline void bind(lua_State* L, const char* name, R(T::*func)(Args...)) {
		typedef functions<T,R,Args...> functions_t;
		typedef typename functions_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&functions_t::f,1);
		lua_setfield(L,-2,name);
	}
	
	static inline void bind(lua_State* L, const char* name, lua_CFunction func) {
		lua_pushcclosure(L,func,0);
		lua_setfield(L,-2,name);
	}

	template <typename ...Args>
	static inline void push(lua_State* L,Args ... rest) {
		int dummy[] = { (S<Args>::push(L,rest),0)... };
		(void)dummy;
	}
}

#endif /*__LLAE_LUABIND_H_INCLUDED__*/