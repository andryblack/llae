#ifndef __LAE_COMMON_REF_COUNTER_H_INCLUDED__
#define __LAE_COMMON_REF_COUNTER_H_INCLUDED__

#include <atomic>
#include <cassert>

namespace common {

	class ref_counter_base {
	private:
		std::atomic<size_t> m_counter;
	protected:
		ref_counter_base() : m_counter(0) {}
		virtual ~ref_counter_base() {
			assert(m_counter == 0);
		}
	public:
		void add_ref() {
			++m_counter;
		}
		void release() {
			if (m_counter.fetch_sub(1) == 1) {
				delete this;
			}
		}
	};

}

#endif /*__COMMON_REF_COUNTER_H_INCLUDED__*/