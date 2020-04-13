#ifndef __LLAE_LUABIND_H_INCLUDED__
#define __LLAE_LUABIND_H_INCLUDED__

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <cstdint>
#include "ref_counter.h"

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
		static void push(lua_State* L,int v) {
			lua_pushinteger(L,v);
		}
	};
	template <>
	struct S<unsigned int> {
		static unsigned int get(lua_State* L,int idx) {
			return lua_tointeger(L,idx);
		}
		static void push(lua_State* L,unsigned int v) {
			lua_pushinteger(L,v);
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
		static void push(lua_State* L,uint64_t idx) {
			lua_pushinteger(L,idx);
		}
	};
	template <>
	struct S<int64_t> {
		static int64_t get(lua_State* L,int idx) {
			return lua_tointeger(L,idx);
		}
		static void push(lua_State* L,int64_t idx) {
			lua_pushinteger(L,idx);
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

	template <class T,class A0,class A1,class A2,class R=void>
	struct func3 {
		typedef R (T::*Func)(lua_State*,A0,A1,A2);
		static int fv(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)(L,S<A0>::get(L,2),S<A1>::get(L,3),S<A2>::get(L,4));
			return 0;
		} 
	};
	template <class T,class A0,class A1,class R=void>
	struct func2 {
		typedef R (T::*Func)(lua_State*,A0,A1);
		typedef R (T::*FuncF)(A0,A1);
		static int fv(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)(L,S<A0>::get(L,2),S<A1>::get(L,3));
			return 0;
		} 
		static int ffv(lua_State* L) {
			FuncF func = *reinterpret_cast<FuncF*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)(S<A0>::get(L,2),S<A1>::get(L,3));
			return 0;
		} 
		static int fr(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			S<R>::push(L,(ref->*func)(L,S<A0>::get(L,2),S<A1>::get(L,3)));
			return 1;
		} 
		static int ffr(lua_State* L) {
			FuncF func = *reinterpret_cast<FuncF*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			S<R>::push(L,(ref->*func)(S<A0>::get(L,2),S<A1>::get(L,3)));
			return 1;
		} 
	};
	template <class T,class A0,class R=void>
	struct func1 {
		typedef R (T::*Func)(lua_State*,A0);
		static int fv(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)(L,S<A0>::get(L,2));
			return 0;
		} 
		static int fr(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			S<R>::push(L,(ref->*func)(L,S<A0>::get(L,2)));
			return 1;
		} 
	};
	template <class T,class R=void>
	struct func0 {
		typedef R (T::*Func)(lua_State*);
		typedef R (T::*FuncF)();
		typedef R (T::*FuncC)()const;
		static int fv(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)(L);
			return 0;
		} 
		static int ffv(lua_State* L) {
			FuncF func = *reinterpret_cast<FuncF*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			(ref->*func)();
			return 0;
		} 
		static int fr(lua_State* L) {
			Func func = *reinterpret_cast<Func*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			S<R>::push(L,(ref->*func)(L));
			return 1;
		} 
		static int ffc(lua_State* L) {
			FuncC func = *reinterpret_cast<FuncC*>(lua_touserdata(L, lua_upvalueindex(1)));
			T* ref = Ref<T>::get_ptr(L,1);
			S<R>::push(L,(ref->*func)());
			return 1;
		} 
	};
	template <class T,class A0,class A1,class A2>
	static inline void bind(lua_State* L, const char* name, void(T::*func)(lua_State* L,A0,A1,A2)) {
		typedef func3<T,A0,A1,A2> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fv,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class A0,class A1>
	static inline void bind(lua_State* L, const char* name, void(T::*func)(lua_State* L,A0,A1)) {
		typedef func2<T,A0,A1> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fv,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class A0,class A1>
	static inline void bind(lua_State* L, const char* name, void(T::*func)(A0,A1)) {
		typedef func2<T,A0,A1> func_t;
		typedef typename func_t::FuncF Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::ffv,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class A0,class A1,class R>
	static inline void bind(lua_State* L, const char* name, R(T::*func)(lua_State* L,A0,A1)) {
		typedef func2<T,A0,A1,R> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fr,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class A0>
	static inline void bind(lua_State* L, const char* name, void(T::*func)(lua_State* L,A0)) {
		typedef func1<T,A0> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fv,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class A0,class R >
	static inline void bind(lua_State* L, const char* name, R(T::*func)(lua_State* L,A0)) {
		typedef func1<T,A0,R> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fr,1);
		lua_setfield(L,-2,name);
	}
	template <class T>
	static inline void bind(lua_State* L, const char* name, void(T::*func)(lua_State* L)) {
		typedef func0<T> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fv,1);
		lua_setfield(L,-2,name);
	}
	template <class T,class R>
	static inline void bind(lua_State* L, const char* name, R(T::*func)(lua_State* L)) {
		typedef func0<T,R> func_t;
		typedef typename func_t::Func Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::fr,1);
		lua_setfield(L,-2,name);
	}
	template <class T>
	static inline void bind(lua_State* L, const char* name, void(T::*func)()) {
		typedef func0<T> func_t;
		typedef typename func_t::FuncF Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::ffv,1);
		lua_setfield(L,-2,name);
	}
	
	template <class T,class R>
	static inline void bind(lua_State* L, const char* name, R(T::*func)()const) {
		typedef func0<T,R> func_t;
		typedef typename func_t::FuncC Func;
		*reinterpret_cast<Func*>(lua_newuserdata(L,sizeof(Func))) = func;
		lua_pushcclosure(L,&func_t::ffc,1);
		lua_setfield(L,-2,name);
	}
	static inline void bind(lua_State* L, const char* name, lua_CFunction func) {
		lua_pushcclosure(L,func,0);
		lua_setfield(L,-2,name);
	}
}

#endif /*__LLAE_LUABIND_H_INCLUDED__*/