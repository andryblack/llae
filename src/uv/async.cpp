#include "async.h"
#include "loop.h"

namespace uv {

	async::async(loop& loop) {
		uv_async_init(loop.native(),&m_async,&async::async_cb);
		attach();
	}

	void async::async_cb(uv_async_t* h) {
		async* self = static_cast<async*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(h)));
		self->on_async();
	}

	int async::send() {
		return uv_async_send(&m_async);
	}
}
