#ifndef _LLAE_LUA_RAW_BIND_H_INCLUDED_
#define _LLAE_LUA_RAW_BIND_H_INCLUDED_

#include "state.h"
#include "stack.h"
#include <new>
#include <tuple>

namespace lua {

	template <typename O,typename F>
	struct field_t {
		using type = F;
		const char* name;
		F O::* ptr;
	};

	template <class MT>
	struct raw_mt {

		using T = typename MT::object;

		template <typename ...F>
		static constexpr std::size_t num_of_fields_i(const std::tuple<F...>& f) {
			return std::tuple_size<std::tuple<F...>>::value;
		}
		static constexpr std::size_t num_of_fields = num_of_fields_i(MT::fields);

		template <typename Val>
		static inline void push_impl(state& l,T* o,Val T::* ptr) {
			::lua::push(l,o->*ptr);
		}
		template <typename Val>
		static inline const char* set_impl(state& l,T* o,Val T::* ptr) {
			o->*ptr = stack<Val>::get(l,3);
			return nullptr;
		}
		
		template <typename F,size_t Len>
		struct array_field_helper {
			F (T::* ptr)[Len];
			static int index(lua_State* L) {
				state s(L);
				auto self = static_cast<array_field_helper*>(s.touserdata(1));
				if (!self) s.argerror(1,"array");
				auto parent = stack<check<raw<MT>>>::get(s,s.upvalueindex(1));
				auto idx = s.checkinteger(2);
				if (idx<0 || idx>=Len) {
					s.argerror(2,"out of bounds");
				}
				::lua::push(s,(parent->*(self->ptr))[idx]);
				return 1;
			}
			static int newindex(lua_State* L) {
				state s(L);
				auto self = static_cast<array_field_helper*>(s.touserdata(1));
				if (!self) s.argerror(1,"array");
				auto parent = stack<check<raw<MT>>>::get(s,s.upvalueindex(1));
				auto idx = s.checkinteger(2);
				if (idx<0 || idx>=Len) {
					s.argerror(2,"out of bounds");
				}
				(parent->*(self->ptr))[idx] = stack<F>::get(s,3);
				return 0;
			}
		};

		template <typename Val,size_t Len>
		static inline void push_impl(state& l,T* o,Val (T::*ptr)[Len]) {
			using field_storage = array_field_helper<Val,Len>;
			new (l.newuserdata(sizeof(field_storage))) field_storage{ptr};
			l.createtable(0,2);
			l.pushvalue(1);
			l.pushcclosure(&field_storage::index,1);
			l.setfield(-2,"__index");
			l.pushvalue(1);
			l.pushcclosure(&field_storage::newindex,1);
			l.setfield(-2,"__newindex");
			l.setmetatable(-2);
		}
		template <typename Val,size_t Len>
		static inline const char* set_impl(state& l,T* o,Val (T::*ptr)[Len]) {
			return "disallow overwrite array field";
		}


		template <std::size_t Idx, bool = true>
		struct apply {
			static constexpr const auto f = std::get<Idx>(MT::fields);
			static bool index(state& l,T* o,const char* fn) {
				if (strcmp(f.name,fn) == 0) {
					push_impl(l,o,f.ptr);
					return true;
				}
				return apply<Idx+1>::index(l,o,fn);
			}
			static const char* newindex(state& l,T* o,const char* fn) {
				if (strcmp(f.name,fn) == 0) {
					return set_impl(l,o,f.ptr);
				}
				return apply<Idx+1>::newindex(l,o,fn);
			}
		};
		template <bool dummy>
		struct apply<num_of_fields,dummy> {
			static bool index(state& l,T* o,const char* fn) {
				return false;
			}
			static const char* newindex(state& l,T* o,const char* fn) {
				return "not found field";
			}
		};

		static int index_impl(lua_State* L) { 
			state s(L);
			const char* fn = s.checkstring(2);
			typename MT::object* o = stack<raw<MT>>::get(s,1);
			if (apply<0>::index(s,o,fn)) {
				return 1;
			}
			s.error("not found field %s on %s",fn,MT::name);
			return 0; 
		}
		static int newindex_impl(lua_State* L) { 
			state s(L);
			const char* fn = s.checkstring(2);
			typename MT::object* o = stack<raw<MT>>::get(s,1);
			auto reason = apply<0>::newindex(s,o,fn);
			if (!reason) {
				return 0;
			}
			s.error("%s %s on %s",reason,fn,MT::name);
			return 0;
		}
		static void bind(state& s) {
			s.pushcclosure(&raw_mt::index_impl,0);
			s.setfield(-2,"__index");
			s.pushcclosure(&raw_mt::newindex_impl,0);
			s.setfield(-2,"__newindex");
		}
	};

	
	
	template <typename O,typename F>
	static constexpr const field_t<O,F> field(const char* name,F O::* ptr) {
		return field_t<O,F>{name,ptr};
	}
	
	template <typename MT>
	static void register_raw_metatable(state& s) {
		if (!s.newmetatable(MT::name)) {
			s.pop(1);
			s.error("metatable %s already registered",MT::name);
		}
		using raw = raw_mt<MT>;
		raw::bind(s);
		s.pop(1);
	}



}

#endif /*_LLAE_LUA_RAW_BIND_H_INCLUDED_*/ 
