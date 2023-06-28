#include "timer.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::timer,uv::handle)
META_OBJECT_INFO(uv::timer_lcb,uv::timer)

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

	timer_pause::timer_pause(lua::state& l) : timer(llae::app::get(l).loop()) {

	}

	void timer_pause::on_cb() {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_cont.release();
            return;
        }
        l.checkstack(2);
        m_cont.push(l);
        auto toth = l.tothread(-1);
        m_cont.reset(l);
        toth.checkstack(1);
        auto s = toth.resume(l,0);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }
        l.pop(1);// thread
	}

	lua::multiret timer_pause::pause(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("timer_pause::pause is async");
			return {2};
		}
		{
			lua_Integer delay = l.checkinteger(1);
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<timer_pause> req{new timer_pause(l)};
			req->m_cont = std::move(cont);
			auto r = req->timer::start(delay,0);
			if (r < 0) {
				req->m_cont.reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}


	void timer_lcb::on_cb() {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_cb.release();
            return;
        }
        l.checkstack(2);
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

	void timer_lcb::lbind(lua::state& l) {
		lua::bind::function(l,"start",&timer_lcb::lstart);
		lua::bind::function(l,"stop",&timer_lcb::lstop);
		lua::bind::function(l,"new",&timer_lcb::lnew);
	}
}