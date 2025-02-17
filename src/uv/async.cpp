#include "async.h"
#include "loop.h"
#include "llae/app.h"
#include "luv.h"
#include "lua/bind.h"

META_OBJECT_INFO(uv::async,uv::handle)
META_OBJECT_INFO(uv::async_continue,uv::async)
META_OBJECT_INFO(uv::async_wait,uv::async_continue)

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

    void async_continue::on_closed() {
        release();
        async::on_closed();
    }

    void async_continue::on_async() {
        auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            release();
            remove_ref();
            return;
        }
        if (!m_cont.valid()) {
            return;
        }
        l.checkstack(2);
        m_cont.push(l);
        reset(l);
        auto toth = l.tothread(-1);
        toth.checkstack(3);
        int nargs = on_cont(toth);
        auto s = toth.resume(l,nargs);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }
        l.pop(1);// thread
    }

    int async_continue::send() {
        int r = async::send();
        if (r<0) {
            return r;
        }
        return r;
    }

    lua::multiret async_wait::lnew(lua::state& l) {
        lua::push(l,common::intrusive_ptr<async_wait>(new async_wait(llae::app::get(l).loop())));
        return {1};
    }
    lua::multiret async_wait::emmit(lua::state& l) {
        auto res = send();
        return return_status_error(l,res);
    }
    int async_wait::on_cont(lua::state& l) {
        l.pushboolean(true);
        return 1;
    }
    lua::multiret async_wait::wait(lua::state& l) {
        if (!l.isyieldable()) {
            l.pushnil();
            l.pushstring("async_wait::wait is async");
            return {2};
        }
        {
            lua::ref cont;
            l.pushthread();
            cont.set(l);
            if (!start(std::move(cont))) {
                l.pushnil();
                l.pushstring("start failed");
                cont.reset(l);
                return {2};
            }
        }
        l.yield(0);
        return {0};
    }

    void async_wait::lbind(lua::state& l) {
        lua::bind::function(l,"new",&async_wait::lnew);
        lua::bind::function(l,"emmit",&async_wait::emmit);
        lua::bind::function(l,"wait",&async_wait::wait);
        lua::bind::function(l,"close",&async_wait::close);
    }

}
