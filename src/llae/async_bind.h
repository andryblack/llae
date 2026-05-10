#pragma once

#include "lua/stack.h"
#include "promise.h"
#include "lua/bind.h"
#include "llae/loop.h"

namespace llae {


    template <typename P,typename R,typename T,typename ... Args>
    struct async_helper;
    template <class P, class R,class T,typename ... Args>
    struct async_helper<P,R,T,lua::state&,Args...> {
        using policy_t = P;
        using result_t = result_promise_ptr<R>;
        typedef result_t (T::*func_t)(lua::state&,Args ... args);
        template <size_t... Is>
        static result_t apply(lua::state&l,T* obj,func_t func,const std::index_sequence<Is...>) {
            return (obj->*func)(l,policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
        }
        static int function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            if (!l.isyieldable()) {
                l.error("is async");
            }
            auto obj = lua::bind::get_self_object<T>(l,1);
            auto p = apply(l,obj,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return p->lawait(l).val;
        }
        static int async_function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            auto obj = lua::bind::get_self_object<T>(l,1);
            auto p = apply(l,obj,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return lua::push(l,std::move(p));
        }
    };

    template <class P, class R,typename ... Args>
    struct async_helper<P,R,void,lua::state&,Args...> {
        using policy_t = P;
        using result_t = result_promise_ptr<R>;
        typedef result_t (*func_t)(lua::state&,Args ... args);
        template <size_t... Is>
        static result_t apply(lua::state&l,func_t func,const std::index_sequence<Is...>) {
            return (*func)(l,policy_t::template arg_policy<Is>::template type<Args>::get(l,1+Is)...);
        }
        static int function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            if (!l.isyieldable()) {
                l.error("is async");
            }
            auto p = apply(l,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return p->lawait(l).val;
        }
        static int async_function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            auto p = apply(l,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return lua::push(l,std::move(p));
        }
    };

    template <class P, class R,class T,typename ... Args>
    struct async_helper<P,R,T,loop&,Args...> {
        using policy_t = P;
        using result_t = result_promise_ptr<R>;
        typedef result_t (T::*func_t)(loop&,Args ... args);
        template <size_t... Is>
        static result_t apply(lua::state&l,T* obj,func_t func,const std::index_sequence<Is...>) {
            return (obj->*func)(loop::get(l),policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
        }
        static int function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            if (!l.isyieldable()) {
                l.error("is async");
            }
            auto obj = lua::bind::get_self_object<T>(l,1);
            auto p = apply(l,obj,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return p->lawait(l).val;
        }
        static int async_function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            auto obj = lua::bind::get_self_object<T>(l,1);
            auto p = apply(l,obj,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return lua::push(l,std::move(p));
        }
    };

    template <class P, class R,typename ... Args>
    struct async_helper<P,R,void,loop&,Args...> {
        using policy_t = P;
        using result_t = result_promise_ptr<R>;
        typedef result_t (*func_t)(loop&,Args ... args);
        template <size_t... Is>
        static result_t apply(lua::state&l,func_t func,const std::index_sequence<Is...>) {
            return (*func)(loop::get(l),policy_t::template arg_policy<Is>::template type<Args>::get(l,1+Is)...);
        }
        static int function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            if (!l.isyieldable()) {
                l.error("is async");
            }
            auto p = apply(l,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return p->lawait(l).val;
        }
        static int async_function(lua_State* L) {
            auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
            lua::state l(L);
            auto p = apply(l,*f,std::index_sequence_for<Args...>());
            if (!p) {
                l.error("async failed");
            }
            return lua::push(l,std::move(p));
        }
    };

    template <class R,class T,typename ... Args>
	static inline void async_function(lua::state& l,const char* name,result_promise_ptr<R> (T::*func)(Args ... args)) {
        using hpr = async_helper<lua::bind::default_func_policy,R,T,Args...>;
        using func_t = typename hpr::func_t;
        func_t* func_data = static_cast<func_t*>(l.newuserdata(sizeof(func_t)));
        *func_data = func;
        l.pushvalue(-1);
        l.pushcclosure(hpr::function,1);
        metatable_set_method(l,name,-3);
        l.pushcclosure(hpr::async_function,1);
        metatable_set_method(l,(std::string("async_")+name).c_str(),-2);
    }

    template <class R,typename ... Args>
	static inline void async_function(lua::state& l,const char* name,result_promise_ptr<R> (*func)(Args ... args)) {
        using hpr = async_helper<lua::bind::default_func_policy,R,void,Args...>;
        using func_t = typename hpr::func_t;
        func_t* func_data = static_cast<func_t*>(l.newuserdata(sizeof(func_t)));
        *func_data = func;
        l.pushvalue(-1);
        l.pushcclosure(hpr::function,1);
        metatable_set_method(l,name,-3);
        l.pushcclosure(hpr::async_function,1);
        metatable_set_method(l,(std::string("async_")+name).c_str(),-2);
    }

}