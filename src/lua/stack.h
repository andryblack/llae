#ifndef __LLAE_LUA_STACK_H_INCLUDED__
#define __LLAE_LUA_STACK_H_INCLUDED__

#include "state.h"
#include "value.h"
#include "meta/object.h"
#include "metatable.h"
#include "common/intrusive_ptr.h"
#include <cstdint>


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


	template <>
	struct stack<int> {
		static int get(state& s,int idx) { return s.tointeger(idx); }
		static void push(state& s,int v) { s.pushinteger(v); }
	};
	template <>
	struct stack<long> {
		static long get(state& s,int idx) { return s.tointeger(idx); }
		static void push(state& s,long v) { s.pushinteger(v); }
	};
	template <>
	struct stack<long long> {
		static long long get(state& s,int idx) { return s.tointeger(idx); }
		static void push(state& s,long long v) { s.pushinteger(v); }
	};
	template <>
	struct stack<unsigned long> {
		static unsigned long get(state& s,int idx) { return s.tointeger(idx); }
		static void push(state& s,unsigned long v) { s.pushinteger(v); }
	};
    template <>
    struct stack<uint32_t> {
        static uint32_t get(state& s,int idx) { return s.tointeger(idx); }
        static void push(state& s,uint32_t v) { s.pushinteger(v); }
    };
    template <>
    struct stack<uint64_t> {
        static uint64_t get(state& s,int idx) { return s.tointeger(idx); }
        static void push(state& s,uint64_t v) { s.pushinteger(v); }
    };
	template <>
	struct stack<const char*> {
		static const char* get(state& s,int idx) { return s.tostring(idx); }
		static void push(state& s,const char* v) { s.pushstring(v); }
	};
	template <>
	struct stack<float> {
		static float get(state& s,int idx) { return s.tonumber(idx); }
		static void push(state& s,float v) { s.pushnumber(v); }
	};
	template <>
	struct stack<double> {
		static double get(state& s,int idx) { return s.tonumber(idx); }
		static void push(state& s,double v) { s.pushnumber(v); }
	};
	template <>
	struct stack<bool> {
		static bool get(state& s,int idx) { return s.toboolean(idx); }
		static void push(state& s,bool v) { s.pushboolean(v); }
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
			push_meta_object(s,std::move(v));
		}
        static void push(state& s,const common::intrusive_ptr<T>& v) {
            if (!v) {
                s.pushnil();
                return;
            }
            push_meta_object(s,v);
        }
	};
	template <class T>
	struct stack<T*> {
		static T* get(state& s,int idx) { 
			auto hdr = static_cast<object_holder_t*>(s.touserdata(idx));
			if (!hdr) return nullptr;
			return hdr->get_raw<T>();
		}
	};
	template <class T>
	struct stack<const common::intrusive_ptr<T>& > : stack<common::intrusive_ptr<T> >{};
	template <class T>
	static void push(state& s,common::intrusive_ptr<T>&& val) {
		stack<common::intrusive_ptr<T> >::push(s,std::move(val));
	}
    template <class T>
    static void push(state& s,const common::intrusive_ptr<T>& val) {
        stack<common::intrusive_ptr<T> >::push(s,val);
    }
}

#endif /*__LLAE_LUA_STACK_H_INCLUDED__*/
