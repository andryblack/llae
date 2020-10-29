#ifndef _LLAE_LUA_REF_H_INCLUDED_
#define _LLAE_LUA_REF_H_INCLUDED_

#include "state.h"
#include <cassert>

namespace lua {

	class ref {
	private:
		int m_ref;
		ref(const ref&) = delete;
		ref& operator = (const ref& ) = delete;
	public:
		ref() : m_ref(LUA_NOREF) {}
		~ref() { assert(m_ref == LUA_NOREF); }
		ref(ref&& r) : m_ref(r.m_ref) {
			r.m_ref = LUA_NOREF;
		}
		ref& operator = (ref&& r) {
			m_ref = r.m_ref;
			r.m_ref = LUA_NOREF;
			return *this;
		}
		bool valid() const { return m_ref != LUA_NOREF; }
		void set(state& l) {
			reset(l);
			m_ref = l.ref();
		}
		void reset(state& l) {
			if (valid()) {
				l.unref(m_ref);
				m_ref = LUA_NOREF;
			}
		}
		void push(state& l) {
			l.pushref(m_ref);
		}
        void release() {
            m_ref = LUA_NOREF;
        }
	};

}

#endif /*_LLAE_LUA_REF_H_INCLUDED_*/ 
