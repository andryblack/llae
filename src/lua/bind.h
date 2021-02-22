#ifndef __LLAE_LUA_BIND_H_INCLUDED__
#define __LLAE_LUA_BIND_H_INCLUDED__

#include "stack.h"
#include "metatable.h"

namespace lua {

	namespace bind {

		template <std::size_t... Is>
	    struct indices {};
	 
	    template <std::size_t N, std::size_t... Is>
	    struct build_indices
	      : build_indices<N-1, N-1, Is...> {};
	 
	    template <std::size_t... Is>
	    struct build_indices<0, Is...> : indices<Is...> {};

		template <typename R,typename T,typename ... Args>
		struct helper {
			typedef R (T::*func_t)(Args ... args);
			typedef R (T::*cfunc_t)(Args ... args) const;
			template <size_t... Is>
			static R apply(state&l,T* obj,func_t func,const indices<Is...>) {
				return (obj->*func)(stack<Args>::get(l,2+Is)...);
			}
			template <size_t... Is>
			static R apply(state&l,const T* obj,cfunc_t func,const indices<Is...>) {
				return (obj->*func)(stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				stack<R>::push(l,apply(l,obj,*f,build_indices<sizeof...(Args)>()));
				return 1;
			}
			static int cfunction(lua_State* L) {
				auto f = static_cast<cfunc_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				stack<R>::push(l,apply(l,obj,*f,build_indices<sizeof...(Args)>()));
				return 1;
			}
		};


		template <>
		struct helper<void,void,state&> {
			typedef void (*func_t)(state&);
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				(*f)(l);
				return 0;
			}
		};


		template <typename ... Args>
		struct helper<void,void,state&,Args...> {
			typedef void (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static void apply(state&l,func_t func,const indices<Is...>) {
				func(l,stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				apply(l,*f,build_indices<sizeof...(Args)>());
				return 0;
			}
		};

		template <typename ... Args>
		struct helper<void,void,Args...> {
			typedef void (*func_t)(Args ... args);
			template <size_t... Is>
			static void apply(state&l,func_t func,const indices<Is...>) {
				func(stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				apply(l,*f,build_indices<sizeof...(Args)>());
				return 0;
			}
		};
		
		template <class T,typename ... Args>
		struct helper<void,T,state&,Args...> {
			typedef void (T::*func_t)(state&,Args ... args);
			template <size_t... Is>
			static void apply(state&l,T* obj,func_t func,const indices<Is...>) {
				(obj->*func)(l,stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				apply(l,obj,*f,build_indices<sizeof...(Args)>());
				return 0;
			}
		};

		template <class T,typename ... Args>
		struct helper<void,T,Args...> {
			typedef void (T::*func_t)(Args ... args);
			template <size_t... Is>
			static void apply(state&l,T* obj,func_t func,const indices<Is...>) {
				(obj->*func)(stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				apply(l,obj,*f,build_indices<sizeof...(Args)>());
				return 0;
			}
		};

		template <class T,typename ... Args>
		struct helper<multiret,T,state&,Args...> {
			typedef multiret (T::*func_t)(state&,Args ... args);
			template <size_t... Is>
			static multiret apply(state&l,T* obj,func_t func,const indices<Is...>) {
				return (obj->*func)(l,stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				auto r = apply(l,obj,*f,build_indices<sizeof...(Args)>());
				return r.val;
			}
		};

		template <class R,class T,typename ... Args>
		struct helper<R,T,state&,Args...> {
			typedef R (T::*func_t)(state&,Args ... args);
			template <size_t... Is>
			static R apply(state&l,T* obj,func_t func,const indices<Is...>) {
				return (obj->*func)(l,stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = stack<T*>::get(l,1);
				if (!obj) {
					l.argerror(1,T::get_class_info()->name);
				}
				stack<R>::push(l,apply(l,obj,*f,build_indices<sizeof...(Args)>()));
				return 1;
			}
		};

		template <typename ... Args>
		struct helper<multiret,void,state&,Args...> {
			typedef multiret (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static multiret apply(state&l,func_t func,const indices<Is...>) {
				return (*func)(l,stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto r = apply(l,*f,build_indices<sizeof...(Args)>());
				return r.val;
			}
		};

		template <class R,typename ... Args>
		struct helper<R,void,state&,Args...> {
			typedef R (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static R apply(state&l,func_t func,const indices<Is...>) {
				return (*func)(l,stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				stack<R>::push(l,apply(l,*f,build_indices<sizeof...(Args)>()));
				return 1;
			}
		};
    
        template <class R,typename ... Args>
        struct helper<R,void,Args...> {
            typedef R (*func_t)(Args ... args);
            template <size_t... Is>
            static R apply(func_t func,const indices<Is...>) {
                return (*func)(stack<Args>::get(1+Is)...);
            }
            static int function(lua_State* L) {
                auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
                state l(L);
                stack<R>::push(l,apply(*f,build_indices<sizeof...(Args)>()));
                return 1;
            }
        };
		
		static void function(state& s,const char* name,int (*func)(lua_State*)) {
			s.pushcclosure(func,0);
			s.setfield(-2,name);
		}
		template <class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(state& l,Args ... args)) {
			typedef helper<R,T,state&,Args...> hpr;
			typedef typename hpr::func_t func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
			s.setfield(-2,name);
		}

		template <class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args)) {
			typedef helper<R,T,Args...> hpr;
			typedef typename hpr::func_t func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
			s.setfield(-2,name);
		}

		template <class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args) const) {
			typedef helper<R,T,Args...> hpr;
			typedef typename hpr::cfunc_t func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::cfunction,1);
			s.setfield(-2,name);
		}

		template <class R,typename ... Args>
		static void function(state& s,const char* name,R (*func)(Args ... args)) {
			typedef helper<R,void,Args...> hpr;
			typedef typename hpr::func_t func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
			s.setfield(-2,name);
		}
    
        template <class T>
        static void value(state& s,const char* name,T v) {
            stack<T>::push(s,v);
            s.setfield(-2,name);
        }


		template <class T>
		struct object {
			static void register_metatable(state& s) {
				create_metatable(s,T::get_class_info());
				s.pop(1);
			} 
			static void register_metatable(state& s,void (*bindfunc)(state&)) {
				create_metatable(s,T::get_class_info());
				bindfunc(s);
				s.pop(1);
			} 
			static void get_metatable(state& s) {
				lua::get_metatable(s,T::get_class_info());
			}
		};
		

	}

	

}

#endif /*__LLAE_LUA_BIND_H_INCLUDED__*/
