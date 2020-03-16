#include "signal.h"
#include "loop.h"

META_OBJECT_INFO(uv::signal,uv::handle)

namespace uv {


	signal::signal(loop& l) {
		int res = uv_signal_init(l.native(),&m_sig);
		if (res >= 0) {
			attach();
		} else {

		}
	}

	signal::~signal() {
	}

	void signal::signal_cb(uv_signal_t* sig, int signum) {
		signal* self = static_cast<signal*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(sig)));
		if (self && self->m_cb) {
			self->m_cb();
		}
	}

	int signal::start_oneshot(int signum,std::function<void()>&& f) {
		m_cb = std::move(f);
		return uv_signal_start_oneshot(&m_sig,&signal::signal_cb, signum);
	}

	int signal::start(int signum,std::function<void()>&& f) {
		m_cb = std::move(f);
		return uv_signal_start(&m_sig,&signal::signal_cb, signum);
	}

}