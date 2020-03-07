#ifndef __LLAE_LUA_STACK_H_INCLUDED__
#define __LLAE_LUA_STACK_H_INCLUDED__

#include "state.h"
#include "value.h"
#include "meta/object.h"
#include "metatable.h"
#include "common/intrusive_ptr.h"
#include <new>

namespace lua {

	template <class T>
	struct stack {};

	template <>
	struct stack<value> {
		static value get(state& s,int idx) {
			return value(s,idx);
		}
		static void push(state& s,const value& v) {
			v.push(s);
		}
	};

	struct object_holder_t {
		common::intrusive_ptr<meta::object> hold;

		const meta::info_t* info() const { return hold->get_object_info(); }
		template <class T>
		common::intrusive_ptr<T> get_intrusive() const {
			return common::intrusive_ptr<T>(meta::cast<T>(hold.get()));
		}
		template <class T>
		T* get_raw() const {
			return meta::cast<T>(hold.get());
		}
	};

	template <>
	struct stack<int> {
		int get(state& s,int idx) { return s.tointeger(idx); }
		void push(state& s,int v) { s.pushinteger(v); }
	};
	template <>
	struct stack<const char*> {
		static const char* get(state& s,int idx) { return s.tostring(idx); }
		static void push(state& s,const char* v) { s.pushstring(v); }
	};
	template <class T>
	struct stack<common::intrusive_ptr<T> > {
		static common::intrusive_ptr<T> get(state& s,int idx) { 
			auto hdr = static_cast<object_holder_t*>(s.touserdata(idx));
			if (!hdr) return common::intrusive_ptr<T>{};
			return hdr->get_intrusive<T>();
		}
		static void push(state& s,common::intrusive_ptr<T>&& v) { 
			if (!v) {
				s.pushnil();
				return;
			}
			void* data = s.newuserdata(sizeof(object_holder_t));
			const meta::info_t* info = v->get_object_info();
			new (data) object_holder_t{ std::move(v) };
			set_metatable(s,info);
		}
	};
	template <class T>
	struct stack<const common::intrusive_ptr<T>& > : stack<common::intrusive_ptr<T> >{};
	template <class T>
	static void push(state& s,common::intrusive_ptr<T>&& val) {
		stack<common::intrusive_ptr<T> >::push(s,std::move(val));
	}
}

#endif /*__LLAE_LUA_STACK_H_INCLUDED__*/