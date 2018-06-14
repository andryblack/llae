#include "work.h"


WorkReq::WorkReq() {
	attach();
}

uv_req_t* WorkReq::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_work);
}

void WorkReq::work_cb(uv_work_t* req) {
	static_cast<WorkReq*>(req->data)->on_work();
}
void WorkReq::after_work_cb(uv_work_t* req,int status) {
	WorkReq* wr = static_cast<WorkReq*>(req->data);
	wr->on_after_work(status);
	wr->remove_ref();
}

int WorkReq::start(uv_loop_t* loop) {
	int r = uv_queue_work(loop,&m_work,&WorkReq::work_cb,&WorkReq::after_work_cb);
	if (r<0) {
		return r;
	}
	add_ref();
	return 0;
}

void ThreadWorkReq::on_after_work(int status) {
	lua_State* L = llae_get_vm(m_work.loop);
	if (L) {
		if (m_th) {
			int res = 1;
			lua_pushboolean(L,status !=0 ? 0 : 1);
			if (status != 0) {
				lua_pushstring(L,uv_strerror(status));
				res = 2;
			}
			m_th.resumevi(L,"ThreadWorkReq::on_after_work",res);
			m_th.reset(L);
		}
	}
}

int ThreadWorkReq::start(lua_State* L) {
	m_th.assign(L);
	int r = WorkReq::start(llae_get_loop(L));
	if (r < 0) {
		m_th.reset(L);
		return r;
	}
	return 0;
}