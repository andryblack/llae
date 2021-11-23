#include "timer.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"

META_OBJECT_INFO(uv::timer,uv::handle)

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

	void timer::timer_cb(uv_timer_t* t) {
		timer* self = static_cast<timer*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(t)));
		if (self) {
			auto is_last = uv_timer_get_repeat(t) == 0;
			self->on_cb();
			if (is_last) {
				self->remove_ref();
			}
		}
	}

	int timer::start(uint64_t timeout, uint64_t repeat) {
		add_ref();
		auto res = uv_timer_start(&m_timer,&timer::timer_cb, timeout, repeat);
		if (res < 0) {
			remove_ref();
		}
		return res;
	}

	int timer::stop() {
		return uv_timer_stop(&m_timer);
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
}