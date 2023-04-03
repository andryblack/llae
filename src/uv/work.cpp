#include "work.h"
#include "loop.h"
#include "llae/app.h"

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

	lua_cont_work::lua_cont_work(lua::ref&& cont) : m_cont(std::move(cont)) {}

	void lua_cont_work::release() {
		m_cont.release();
	}

	void lua_cont_work::reset(lua::state& l) {
		m_cont.reset(l);
	}

	int lua_cont_work::queue_work(lua::state& l) {
		return work::queue_work(llae::app::get(l).loop());
	}

	int lua_cont_work::queue_work_thread(lua::state& l) {
		l.pushthread();
        m_cont.set(l);
        auto r = work::queue_work(llae::app::get(l).loop());
        if (r < 0) {
        	reset(l);
        }
        return r;
	}

	void lua_cont_work::on_after_work(int status) {
		auto& l(llae::app::get(get_loop()).lua());
        if (!l.native()) {
            m_cont.release();
            return;
        }
        if (m_cont.valid()) {
            m_cont.push(l);
            
            auto toth = l.tothread(-1);
            lua::ref rref = std::move(m_cont);
            l.pop(1);// thread

            auto args = resume_args(toth,status);

            auto s = toth.resume(l,args);
            
            if (s != lua::status::ok && s != lua::status::yield) {
                llae::app::show_error(toth,s);
            }

            
        }
	}
}
