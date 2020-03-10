#include "loop.h"

META_OBJECT_INFO(uv::loop,meta::object)

namespace uv {

	loop::loop(uv_loop_t* l) : m_loop(l)  {
	}

	loop::~loop() {
	}

	int loop::run(uv_run_mode mode) {
		return uv_run(m_loop,mode);
	}

	bool loop::is_alive() const {
		return uv_loop_alive(m_loop);
	}

	uint64_t loop::now() const {
		return uv_now(m_loop);
	}

	void loop::stop() {
		uv_stop(m_loop);
	}
}