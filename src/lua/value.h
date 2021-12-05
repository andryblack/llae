#ifndef _LLAE_LUA_VALUE_H_INCLUDED_
#define _LLAE_LUA_VALUE_H_INCLUDED_

#include "state.h"

namespace lua {

	class value {
	protected:
		state& m_state;
		int m_idx;
	public:
		explicit value( state& st, int idx ) : m_state(st), m_idx(idx) {
			if (idx < 0) { m_idx = st.gettop()+m_idx+1; }
		}
		state& get_state() { return m_state; }
		value_type get_type() const { return m_state.get_type(m_idx); }
		bool is_none() const 	{ return get_type() == value_type::none; }
		bool is_nil() const 	{ return get_type() == value_type::lnil; }
		bool is_number() const 	{ return get_type() == value_type::number; }
		bool is_boolean() const { return get_type() == value_type::boolean; }
		bool is_string() const 	{ return get_type() == value_type::string; }
		bool is_table() const 	{ return get_type() == value_type::table; }
		bool is_function() const { return get_type() == value_type::function; }
		bool is_userdata() const { return get_type() == value_type::userdata; }
		bool is_thread() const 	{ return get_type() == value_type::thread; }
		bool is_lightuserdata() const { return get_type() == value_type::lightuserdata; }
		
		const char* tostring() const { return m_state.tostring(m_idx);}
		void push() const {
			m_state.pushvalue(m_idx);
		}
		void push(state& s) const {
			push();
			if (s!=m_state) {
				s.xmove(m_state);
			}
		}
	};

}

#endif /*_LLAE_LUA_VALUE_H_INCLUDED_*/
