#ifndef _LUA_VALUE_H_INCLUDED_
#define _LUA_VALUE_H_INCLUDED_

#include "lua_state.h"

namespace Lua {

	class StackValue {
	protected:
		StateRef& m_state;
		int m_idx;
	public:
		explicit StackValue( StateRef& st, int idx ) : m_state(st), m_idx(idx) {}
		ValueType get_type() const { return m_state.get_type(m_idx); }
		bool is_none() const 	{ return get_type() == ValueType::NONE; }
		bool is_nil() const 	{ return get_type() == ValueType::NIL; }
		bool is_number() const 	{ return get_type() == ValueType::NUMBER; }
		bool is_boolean() const { return get_type() == ValueType::BOOLEAN; }
		bool is_string() const 	{ return get_type() == ValueType::STRING; }
		bool is_table() const 	{ return get_type() == ValueType::TABLE; }
		bool is_function() const { return get_type() == ValueType::FUNCTION; }
		bool is_userdata() const { return get_type() == ValueType::USERDATA; }
		bool is_thread() const 	{ return get_type() == ValueType::THREAD; }
		bool is_lightuserdata() const { return get_type() == ValueType::LIGHTUSERDATA; }

		const char* tostring() const { return m_state.tostring(m_idx);}
	};

}

#endif /*_LUA_VALUE_H_INCLUDED_*/