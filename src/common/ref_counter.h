#ifndef __LAE_COMMON_REF_COUNTER_H_INCLUDED__
#define __LAE_COMMON_REF_COUNTER_H_INCLUDED__

#include <atomic>
#include <cassert>

namespace common {

	class ref_counter_base {
	private:
		std::atomic<size_t> m_counter;
		ref_counter_base(const ref_counter_base&) = delete;
		ref_counter_base& operator = (const ref_counter_base&) = delete;
	protected:
		ref_counter_base() : m_counter(0) {}
		virtual ~ref_counter_base() {
			assert(m_counter == 0);
		}
		virtual void destroy() {
			delete this;
		}
	public:
		void add_ref() {
			++m_counter;
		}
		void remove_ref() {
            assert(m_counter > 0);
			if (m_counter.fetch_sub(1) == 1) {
				destroy();
			}
		}
	};

}

#endif /*__COMMON_REF_COUNTER_H_INCLUDED__*/
