#ifndef __LLAE_LUA_STACK_H_INCLUDED__
#define __LLAE_LUA_STACK_H_INCLUDED__

#include "state.h"
#include "value.h"

namespace lua {

	template <class T>
	struct stack {};

	template <>
	struct stack<value> {
		value get(state& s,int idx) {
			return value(s,idx);
		}
		void push(state& s,const value& v) {
			v.push(s);
		}
	};

	template <>
	struct stack<int> {
		int get(state& s,int idx) { return s.tointeger(idx); }
		void push(state& s,int v) { s.pushinteger(v); }
	};
}

#endif /*__LLAE_LUA_STACK_H_INCLUDED__*/