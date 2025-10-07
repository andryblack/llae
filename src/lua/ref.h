#pragma once

#include "state.h"

namespace lua {

	class ref {
	private:
		int m_ref;
		ref(const ref&) = delete;
		ref& operator = (const ref& ) = delete;
	public:
		ref() : m_ref(LUA_NOREF) {}
        ~ref();
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
			if (!l.native()) {
				release();
			} else {
				if (valid()) {
					l.unref(m_ref);
					m_ref = LUA_NOREF;
				}
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
