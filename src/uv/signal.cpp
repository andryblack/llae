#include "signal.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::signal_base,uv::handle)
META_OBJECT_INFO(uv::signal,uv::signal_base)
META_OBJECT_INFO(uv::lua_signal,uv::signal_base)

namespace uv {


	signal_base::signal_base(loop& l) {
		int res = uv_signal_init(l.native(),&m_sig);
		if (res >= 0) {
			attach();
		} else {

		}
	}

	void signal_base::signal_cb(uv_signal_t* sig, int signum) {
		signal_base* self = static_cast<signal_base*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(sig)));
		if (self) {
			self->on_signal(signum);
		}
	}

	signal_base::~signal_base() {
	}

	int signal_base::start_oneshot(int signum) {
		return uv_signal_start_oneshot(&m_sig,&signal_base::signal_cb, signum);
	}

	int signal_base::start(int signum) {
		return uv_signal_start(&m_sig,&signal_base::signal_cb, signum);
	}

	void signal::on_signal(int signum) {
		if (m_cb) {
			m_cb(signum);
		}
	}

	int signal::start_oneshot(int signum,std::function<void(int)>&& f) {
		m_cb = std::move(f);
		return signal_base::start_oneshot(signum);
	}

	int signal::start(int signum,std::function<void(int)>&& f) {
		m_cb = std::move(f);
		return signal_base::start(signum);
	}


	lua::multiret lua_signal::start_oneshot(lua::state& l, int signum, int i) {
		l.pushvalue(i);
		m_ref.set(l);
		add_ref();
		auto res = signal_base::start_oneshot(signum);
		if (res < 0) {
			m_ref.reset(l);
			unref();
			return return_status_error(l,res);
		}
		l.pushboolean(true);
		return {1};
	}

 	lua::multiret lua_signal::oneshot(lua::state& l) {
 		auto signum = l.tointeger(1);
 		l.checktype(2,lua::value_type::function);
 		common::intrusive_ptr<lua_signal> sig{new lua_signal(llae::app::get(l).loop())};
 		return sig->start_oneshot(l,signum,2);
	}

	void lua_signal::on_signal(int signum) {
		auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            m_ref.release();
            return;
        }
        l.checkstack(2);
        m_ref.push(l);
        m_ref.reset(l);
        l.pushinteger(signum);
        auto s = l.pcall(1,0,0);
        if (s != lua::status::ok) {
            llae::app::show_error(l,s);
        }
        unref();
	}

	void lua_signal::lbind(lua::state& l) {
		lua::bind::function(l,"oneshot",&lua_signal::oneshot);
	}

}