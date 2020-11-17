#include "mutex.h"

namespace uv {

	mutex::mutex() {
		uv_mutex_init(&m_m);
	}

	mutex::~mutex() {
		uv_mutex_destroy(&m_m);
	}

	void mutex::lock() {
		uv_mutex_lock(&m_m);
	}

	void mutex::unlock() {
		uv_mutex_unlock(&m_m);
	}

}