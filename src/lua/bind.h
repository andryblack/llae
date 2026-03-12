#pragma once

#include "stack.h"
#include "metatable.h"
#include "array.h"
#include "policy.h"
#include <utility>
#include <algorithm>

namespace lua {

	namespace bind {

		template <typename T>
		static T* get_self_object(state& l,int idx) {
			auto obj = meta_holder_base_t::get_ptr<T>(l,idx);
			if (!obj.first) {
				l.error("invalid self object %s",meta::info<T>::get()->name);
			}
			if (!std::is_const<T>::value && obj.second) {
				l.error("invalid self object %s is const",meta::info<T>::get()->name);
			}
			return obj.first;
		}

		

		template <typename P,typename R,typename T,typename ... Args>
		struct helper {
			using policy_t = P;
			using func_t = R (T::*)(Args ... args);
			using cfunc_t = R (T::*)(Args ... args) const;
			template <typename O,typename F,size_t... Is>
			static R apply(state&l,O* obj,F func,const std::index_sequence<Is...>) {
				return (obj->*func)(policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<T>(l,1);
				return policy_t::push_result(l,apply(l,obj,*f,std::index_sequence_for<Args...>()));
			}
			static int cfunction(lua_State* L) {
				auto f = static_cast<cfunc_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<const T>(l,1);
				return policy_t::push_result(l,apply(l,obj,*f,std::index_sequence_for<Args...>()));
			}
		};


		template <typename P>
		struct helper<P,void,void,state&> {
			typedef void (*func_t)(state&);
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				(*f)(l);
				return 0;
			}
		};


		template <typename P,typename ... Args>
		struct helper<P,void,void,state&,Args...> {
			typedef void (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static void apply(state&l,func_t func,const std::index_sequence<Is...>) {
				func(l,stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				apply(l,*f,std::index_sequence_for<Args...>());
				return 0;
			}
		};

		template <typename P,typename ... Args>
		struct helper<P,void,void,Args...> {
			typedef void (*func_t)(Args ... args);
			template <size_t... Is>
			static void apply(state&l,func_t func,const std::index_sequence<Is...>) {
				func(stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				apply(l,*f,std::index_sequence_for<Args...>());
				return 0;
			}
		};
		
		template <typename P,class T,typename ... Args>
		struct helper<P,void,T,state&,Args...> {
			using policy_t = P;
			using func_t = void (T::*)(state&,Args ... args);
			template <size_t... Is>
			static void apply(state&l,T* obj,func_t func,const std::index_sequence<Is...>) {
				(obj->*func)(l,policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
			}
            template <size_t... Is>
            static T* apply_ctr(state&l,const std::index_sequence<Is...>) {
                return new T(l,stack<Args>::get(l,1+Is)...);
            }
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<T>(l,1);
				apply(l,obj,*f,std::index_sequence_for<Args...>());
				return 0;
			}
            static int ctr(lua_State* L) {
                state l(L);
                common::intrusive_ptr<T> res{apply_ctr(l,std::index_sequence_for<Args...>())};
                stack<common::intrusive_ptr<T> >::push(l,std::move(res));
                return 1;
            }
		};

		template <typename P,class T,typename ... Args>
		struct helper<P,void,T,Args...> {
			using policy_t = P;
			using func_t = void (T::*)(Args ... args);
			using cfunc_t = void (T::*)(Args ... args) const;
			template <typename O,typename F,size_t... Is>
			static void apply(state&l,O* obj,F func,const std::index_sequence<Is...>) {
				(obj->*func)(policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
			}
            template <size_t... Is>
            static T* apply_ctr(state&l,const std::index_sequence<Is...>) {
                return new T(stack<Args>::get(l,1+Is)...);
            }
			template <size_t... Is>
            static void apply_inplace_ctr(state&l,const std::index_sequence<Is...>) {
				construct_raw<T,Args...>(l,stack<Args>::get(l,1+Is)...);
            }
			
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<T>(l,1);
				apply(l,obj,*f,std::index_sequence_for<Args...>());
				return 0;
			}
			static int cfunction(lua_State* L) {
				auto f = static_cast<cfunc_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<const T>(l,1);
				apply(l,obj,*f,std::index_sequence_for<Args...>());
				return 0;
			}
            static int ctr(lua_State* L) {
                state l(L);
                common::intrusive_ptr<T> res{apply_ctr(l,std::index_sequence_for<Args...>())};
                stack<common::intrusive_ptr<T> >::push(l,std::move(res));
                return 1;
            }
            static int raw_ctr(lua_State* L) {
                state l(L);
                apply_inplace_ctr(l,std::index_sequence_for<Args...>());
                return 1;
            }
		};

		template <class T,typename ... Args>
		struct helper<default_func_policy,multiret,T,state&,Args...> {
			typedef multiret (T::*func_t)(state&,Args ... args);
			typedef multiret (T::*cfunc_t)(state&,Args ... args)const;
			template <typename O,typename F,size_t... Is>
			static multiret apply(state&l,O* obj,F func,const std::index_sequence<Is...>) {
				return (obj->*func)(l,stack<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<T>(l,1);
				auto r = apply(l,obj,*f,std::index_sequence_for<Args...>());
				return r.val;
			}
			static int cfunction(lua_State* L) {
				auto f = static_cast<cfunc_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<const T>(l,1);
				auto r = apply(l,obj,*f,std::index_sequence_for<Args...>());
				return r.val;
			}
		};

		template <typename P,class R,class T,typename ... Args>
		struct helper<P,R,T,state&,Args...> {
			using policy_t = P;
			typedef R (T::*func_t)(state&,Args ... args);
			template <size_t... Is>
			static R apply(state&l,T* obj,func_t func,const std::index_sequence<Is...>) {
				return (obj->*func)(l,policy_t::template arg_policy<Is>::template type<Args>::get(l,2+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto obj = get_self_object<T>(l,1);
				return policy_t::push_result(l,apply(l,obj,*f,std::index_sequence_for<Args...>()));
			}
		};

		template <typename ... Args>
		struct helper<default_func_policy,multiret,void,state&,Args...> {
			typedef multiret (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static multiret apply(state&l,func_t func,const std::index_sequence<Is...>) {
				return (*func)(l,stack<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				auto r = apply(l,*f,std::index_sequence_for<Args...>());
				return r.val;
			}
		};

		template <typename P,class R,typename ... Args>
		struct helper<P,R,void,state&,Args...> {
			using policy_t = P;
			typedef R (*func_t)(state&,Args ... args);
			template <size_t... Is>
			static R apply(state&l,func_t func,const std::index_sequence<Is...>) {
				return (*func)(l,policy_t::template arg_policy<Is>::template type<Args>::get(l,1+Is)...);
			}
			static int function(lua_State* L) {
				auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				state l(L);
				stack<R>::push(l,apply(l,*f,std::index_sequence_for<Args...>()));
				return 1;
			}
		};
    
        template <typename P,class R,typename ... Args>
        struct helper<P,R,void,Args...> {
			using policy_t = P;
            typedef R (*func_t)(Args ... args);
            template <size_t... Is>
            static R apply(state&l,func_t func,const std::index_sequence<Is...>) {
                return (*func)(policy_t::template arg_policy<Is>::template type<Args>::get(l,1+Is)...);
            }
            static int function(lua_State* L) {
                auto f = static_cast<func_t*>(lua_touserdata(L,lua_upvalueindex(1)));
                state l(L);
                return policy_t::push_result(l,apply(l,*f,std::index_sequence_for<Args...>()));
            }
        };

		template <class R,class T,typename P = default_field_policy>
		struct field_helper {
			using policy_t = P;
			using field_t = R (T::*);
			static int get(lua_State* L) {
				state s(L);
				auto obj = meta_holder_base_t::get_ptr<T>(s,1);
				if (!obj.first) {
					s.error("invalid self object %s",meta::info<T>::get()->name);
				}
				auto field = *static_cast<field_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				if (obj.second) {
					return policy_t::template push_field<const R>(s,obj.first->*field);
				} else {
					return policy_t::template push_field<R>(s,obj.first->*field);
				}
			}
			static int set(lua_State* L) {
				state s(L);
				auto obj = get_self_object<T>(s,1);
				auto field = *static_cast<field_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				policy_t::set_field(s,obj->*field,2);
				return 0;
			}
		};


		template <typename R,class T, size_t size, typename P>
		struct field_helper<R[size],T,P> {
			using policy_t = P;
			using field_t = R (T::*)[size];
			static int get(lua_State* L) {
				state s(L);
				auto obj = meta_holder_base_t::get_ptr<T>(s,1);
				if (!obj.first) {
					s.error("invalid self object %s",meta::info<T>::get()->name);
				}
				auto field = *static_cast<field_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				if (obj.second) {
					push_array_ref<policy_t,const R>(s,obj.first->*field,size,1);
				} else {
					push_array_ref<policy_t,R>(s,obj.first->*field,size,1);
				}
				ref_value(s,-1,1);
				return 1;
			}
			static int set(lua_State* L) {
				state s(L);
				s.error("attempt to set array field");
				return 0;
			}
		};

		template <typename R,class T, size_t size, bool zero_terminate>
		struct field_helper<R[size],T,string_policy<zero_terminate>> {
			using policy_t = string_policy<zero_terminate>;
			using field_t = R (T::*)[size];
			static int get(lua_State* L) {
				state s(L);
				auto obj = meta_holder_base_t::get_ptr<T>(s,1);
				if (!obj.first) {
					s.error("invalid self object %s",meta::info<T>::get()->name);
				}
				auto field = *static_cast<field_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				return policy_t::push_field(s,obj.first->*field);
			}
			static int set(lua_State* L) {
				state s(L);
				auto obj = get_self_object<T>(s,1);
				auto field = *static_cast<field_t*>(lua_touserdata(L,lua_upvalueindex(1)));
				policy_t::set_field(s,obj->*field,2);
				return 0;
			}
		};
		
		static void function(state& s,const char* name,int (*func)(lua_State*)) {
			s.pushcclosure(func,0);
            metatable_set_method(s,name,-2);
		}
		// template <class R,class T,typename ... Args>
		// static void function(state& s,const char* name,R (T::*func)(state& l,Args ... args)) {
		// 	using hpr = helper<default_policy,R,T,state&,Args...>;
		// 	using func_t = typename hpr::func_t; 
		// 	func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
		// 	*func_data = func;
		// 	s.pushcclosure(hpr::function,1);
        //     metatable_set_method(s,name,-2);
		// }

		template <class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args)) {
			using hpr = helper<default_func_policy,R,T,Args...>;
			using func_t = typename hpr::func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
            metatable_set_method(s,name,-2);
		}

		template <class P,class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args),P) {
			static_assert(P::type == policy_type::func, "policy must be a function policy");
			using hpr = helper<P,R,T,Args...>;
			using func_t = typename hpr::func_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
            metatable_set_method(s,name,-2);
		}

		template <class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args) const) {
			using hpr = helper<default_func_policy,R,T,Args...>;
			using func_t = typename hpr::cfunc_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::cfunction,1);
            metatable_set_method(s,name,-2);
		}

		template <class P,class R,class T,typename ... Args>
		static void function(state& s,const char* name,R (T::*func)(Args ... args) const,P) {
			static_assert(P::type == policy_type::func, "policy must be a function policy");
			using hpr = helper<P,R,T,Args...>;
			using func_t = typename hpr::cfunc_t; 
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::cfunction,1);
            metatable_set_method(s,name,-2);
		}

		template <class R,typename ... Args>
		static void function(state& s,const char* name,R (*func)(Args ... args)) {
			using hpr = helper<default_func_policy,R,void,Args...>;
			using func_t = typename hpr::func_t;
			func_t* func_data = static_cast<func_t*>(s.newuserdata(sizeof(func_t)));
			*func_data = func;
			s.pushcclosure(hpr::function,1);
            metatable_set_method(s,name,-2);
		}

		template <class R,class T>
		static void field_ro(state& s,const char* name,R (T::*field)) {
			using hpr = field_helper<R,T>;
			using field_t = typename hpr::field_t;
			field_t* field_data = static_cast<field_t*>(s.newuserdata(sizeof(field_t)));
			*field_data = field;
			s.pushcclosure(hpr::get,1);
			metatable_set_getter(s,name,-2);
		}

		template <class R,class T>
		static void field(state& s,const char* name,R (T::*field)) {
			using hpr = field_helper<R,T>;
			using field_t = typename hpr::field_t;
			field_t* field_data = static_cast<field_t*>(s.newuserdata(sizeof(field_t)));
			*field_data = field;
			s.pushvalue(-1);
			s.pushcclosure(hpr::get,1);
			metatable_set_getter(s,name,-3);
			s.pushcclosure(hpr::set,1);
			metatable_set_setter(s,name,-2);
		}

		template <class R,class T,typename P>
		static void field(state& s,const char* name,R (T::*field),P p) {
			static_assert(P::type == policy_type::field, "policy must be a field policy");
			using hpr = field_helper<R,T,P>;
			using field_t = typename hpr::field_t;
			field_t* field_data = static_cast<field_t*>(s.newuserdata(sizeof(field_t)));
			*field_data = field;
			s.pushvalue(-1);
			s.pushcclosure(hpr::get,1);
			metatable_set_getter(s,name,-3);
			s.pushcclosure(hpr::set,1);
			metatable_set_setter(s,name,-2);
		}
    
        template <class T,typename ... Args>
        static void constructor(state& s) {
            typedef helper<default_func_policy,void,T,Args...> hpr;
            s.pushcclosure(hpr::ctr,0);
            metatable_set_method(s,"new",-2);
        }
		
		template <class T,typename ... Args>
        static void raw_constructor(state& s) {
            typedef helper<default_func_policy,void,T,Args...> hpr;
            s.pushcclosure(hpr::raw_ctr,0);
			metatable_set_method(s,"new",-2);
        }
    
        template <class T>
        static void value(state& s,const char* name,T v) {
            stack<T>::push(s,v);
            metatable_set_field(s,name,-2);
        }


		template <class T>
		struct object {
			static void register_metatable(state& s) {
				create_metatable(s,meta::info<T>::get());
				s.pop(1);
			} 
			static void register_metatable(state& s,void (*bindfunc)(state&)) {
				create_metatable(s,meta::info<T>::get());
				bindfunc(s);
				s.pop(1);
			} 
			static bool get_metatable(state& s) {
				return lua::get_metatable(s,meta::info<T>::get());
			}
		};
    

	}

	

}
