#ifndef __LLAE_UV_MUTEX_H_INCLUDED__
#define __LLAE_UV_MUTEX_H_INCLUDED__

#include "common/ref_counter.h"
#include "decl.h"

namespace uv {

	class mutex {
	private:
		uv_mutex_t m_m;
		mutex(const mutex&) = delete;
		mutex(mutex&&) = delete;
		mutex& operator = (const mutex&) = delete;
		mutex& operator = (mutex&&) = delete;
	public:
		mutex();
		~mutex();
		void lock();
		void unlock();
	};

	class scoped_lock {
	private:
		mutex& m_m;
	public:
		explicit scoped_lock(mutex& m) : m_m(m) {
			m_m.lock();
		}
		~scoped_lock() {
			m_m.unlock();
		}
	};

}

#endif /*__LLAE_UV_MUTEX_H_INCLUDED__*/