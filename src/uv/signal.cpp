#include "signal.h"
#include "loop.h"

META_OBJECT_INFO(uv::signal,uv::handle)

namespace uv {


	signal::signal(loop& l) {
		int res = uv_signal_init(l.native(),&m_sig);
	}

}