#include "timer.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"

META_OBJECT_INFO(uv::timer,uv::handle)
META_OBJECT_INFO(uv::timer_lcb,uv::timer)
META_OBJECT_INFO(uv::timer_wait,uv::timer)

namespace uv {


	timer::timer(loop& l) {
		int res = uv_timer_init(l.native(),&m_timer);
		if (res >= 0) {
			attach();
		} else {

		}
	}

	timer::~timer() {
	}

	void timer::unref() {
		if (m_start_ref) {
			m_start_ref = false;
			remove_ref();
		}
	}
	void timer::timer_cb(uv_timer_t* t) {
		timer* self = static_cast<timer*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(t)));
		if (self) {
			auto is_last = uv_timer_get_repeat(t) == 0;
			self->on_cb();
			if (is_last) {
				self->unref();
			}
		} else {
			uv_timer_stop(t);
		}
	}

	int timer::start(uint64_t timeout, uint64_t repeat) {
		if (!m_start_ref) {
			add_ref();
			m_start_ref = true;
		}
		auto res = uv_timer_start(&m_timer,&timer::timer_cb, timeout, repeat);
		if (res < 0) {
			unref();
		}
		return res;
	}

	int timer::stop() {
		auto res = uv_timer_stop(&m_timer);
		unref();
		return res;
	}

	void timer_pause::on_cb() {
		if (m_promise) {
			m_promise->set_result(llae::result<>());
			m_promise.reset();
		}
	}

	llae::result_promise_ptr<void> timer_pause::pause(llae::loop& l,unsigned int delay) {
		llae::result_promise_ptr<void> result = common::make_intrusive<llae::result_promise<void>>();
		common::intrusive_ptr<timer_pause> req{new timer_pause(static_cast<loop&>(l),result)};
		auto res = req->start(delay,0);
		if (res < 0) {
			result->set_result(make_result(res));
		}
		return result;
	}

	timer_delayed_resume::timer_delayed_resume(lua::state& l) : timer(llae::app::get(l).loop()) {
	}

	void timer_delayed_resume::on_cb() {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_cont.release();
            return;
        }
        l.checkstack(2);
        m_cont.push(l);
        auto toth = l.tothread(-1);
        auto cont = std::move(m_cont);
        l.pop(1);// thread
        toth.checkstack(1);
        auto s = toth.resume(l,0);
        cont.reset(l);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }

	}
		

	llae::result<void> timer_delayed_resume::resume_delayed(lua::state& l) {
		l.checktype(1, lua::value_type::thread);
		{
			lua_Integer delay = l.optinteger(2,0);
			lua::ref cont;
			l.pushvalue(1);
			cont.set(l);
			common::intrusive_ptr<timer_delayed_resume> req{new timer_delayed_resume(l)};
			req->m_cont = std::move(cont);
			auto r = req->timer::start(delay,0);
			if (r < 0) {
				req->m_cont.reset(l);
				return make_result(r);
			} 
		}
		return llae::result<void>{};
	}

	void timer_lcb::on_cb() {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_cb.release();
            return;
        }
        l.checkstack(3);
        m_cb.push(l);
        lua::push(l,timer_ptr(this));
        auto s = l.pcall(1,0,0);
        if (s != lua::status::ok) {
            llae::app::show_error(l,s);
        }
	}

	void timer_lcb::on_closed() {
        if (llae::app::closed(get_handle()->loop)) {
            m_cb.release();
        } else {
            m_cb.reset(llae::app::get(get_handle()->loop).lua());
        }
	}

	lua::multiret timer_lcb::lnew(lua::state& l) {
		common::intrusive_ptr<timer_lcb> req{new timer_lcb(llae::app::get(l).loop())};
		lua::push(l,std::move(req));
		return {1};
	}

	lua::multiret timer_lcb::lstart(lua::state& l) {
		l.checktype(2,lua::value_type::function);
		l.pushvalue(2);
		m_cb.set(l);
		auto timeout = l.checkinteger(3);
		auto repeat = l.optinteger(4,0);
		auto r = start(timeout,repeat);
		return return_status_error(l,r);
	}

	lua::multiret timer_lcb::lstop(lua::state& l) {
		auto r = stop();
		m_cb.reset(l);
		return return_status_error(l,r);
	}


	void timer_wait::resume(lua::state& l,const char* status) {
        l.checkstack(2);
        m_cont.push(l);
        auto toth = l.tothread(-1);
        m_cont.reset(l);
        toth.checkstack(3);
        int rets;
        if (status) {
        	toth.pushnil();
        	toth.pushstring(status);
        	rets = 2;
        } else {
        	toth.pushboolean(true);
        	rets = 1;
        }
        auto s = toth.resume(l,rets);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }
        l.pop(1);// thread
	}
	void timer_wait::on_cb() {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_cont.release();
            return;
        }
        if (m_cont.valid()) {
        	m_ready = false;
        	resume(l,nullptr);
	    } else {
	    	m_ready = true;
	    }
	}

	void timer_wait::on_closed() {
        if (llae::app::closed(get_handle()->loop)) {
            m_cont.release();
        } else {
            m_cont.reset(llae::app::get(get_handle()->loop).lua());
        }
	}

	lua::multiret timer_wait::lnew(lua::state& l) {
		common::intrusive_ptr<timer_wait> req{new timer_wait(llae::app::get(l).loop())};
		lua::push(l,std::move(req));
		return {1};
	}

	llae::result<> timer_wait::lstart(lua::state& l,int timeout) {
		m_ready = false;
		if (m_started) {
			return llae::result<>(llae::string_error::create("already started"));
		}
		m_started = true;
		auto repeat = l.optinteger(3,0);
		auto r = start(timeout,repeat);
		return make_result(r);
	}

	llae::result<> timer_wait::lstop(lua::state& l) {
		if (!m_started) {
			return llae::result<>(llae::string_error::create("not started"));
		}
		m_ready = false;
		m_started = false;
		auto r = stop();
		if (m_cont.valid()) {
			resume(l,"stop");
		}
		return make_result(r);
	}

	lua::multiret timer_wait::lwait(lua::state& l) {
		if (!m_started) {
			l.pushnil();
			l.pushstring("not started");
			return {2};
		}
		if (m_ready) {
			m_ready = false;
			l.pushboolean(true);
			return {1};
		}
		if (m_cont.valid()) {
			l.pushnil();
			l.pushstring("already wait");
			return {2};
		}
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("timer_wait::lwait is async");
			return {2};
		}
		{
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			m_cont = std::move(cont);
		}
		l.yield(0);
		return {0};
	}

}