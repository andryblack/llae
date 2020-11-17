#include "work.h"
#include "loop.h"

namespace uv {

	work::work() {
		attach(reinterpret_cast<uv_req_t*>(&m_work));
	}

	work::~work() {

	}

	void work::work_cb(uv_work_t* req) {
		work* self = static_cast<work*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_work();
	}

	void work::after_work_cb(uv_work_t* req,int status) {
		work* self = static_cast<work*>(uv_req_get_data(reinterpret_cast<uv_req_t*>(req)));
		self->on_after_work(status);
		self->remove_ref();
	}

	int work::queue_work(loop& l) {
		add_ref();
		int r = uv_queue_work(l.native(),&m_work,&work::work_cb,&work::after_work_cb);
		if (r < 0) {
			remove_ref();
		}
		return r;
	}
}
