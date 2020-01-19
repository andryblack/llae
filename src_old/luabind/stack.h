#ifndef __LUABIND_STACK_H_INCLUDED__
#define __LUABIND_STACK_H_INCLUDED__

#include "lua_state.h"
#include "lua_value.h"

namespace luabind {

	template <class T>
	struct S {};

	template <>
	struct S<value> {
		value get(state& s,int idx) {
			return value(s,idx);
		}
		void push(state& s,const value& v) {
			v.push(s);
		}
	};
}

#endif /*__LUABIND_STACK_H_INCLUDED__*/