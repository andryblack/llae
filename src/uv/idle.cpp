#include "idle.h"
#include "loop.h"

META_OBJECT_INFO(uv::idle,uv::handle)


namespace uv {

	idle::idle(loop& l) {
		int res = uv_idle_init(l.native(),&m_idle);
		if (res >= 0) {
			attach();
		} else {

		}
	}

	idle::~idle() {
	}

	void idle::unref() {
		if (m_start_ref) {
			m_start_ref = false;
			remove_ref();
		}
	}

	void idle::idle_cb(uv_idle_t* t) {
		idle* self = static_cast<idle*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(t)));
		if (self) {
			self->on_cb();
		} else {
			uv_idle_stop(t);
		}
	}

	int idle::start() {
		if (!m_start_ref) {
			add_ref();
			m_start_ref = true;
		}
		auto res = uv_idle_start(&m_idle,&idle::idle_cb);
		if (res < 0) {
			unref();
		}
		return res;
	}

	int idle::stop() {
		auto res = uv_idle_stop(&m_idle);
		unref();
		return res;
	}

}