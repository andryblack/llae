#include "async.h"
#include "loop.h"
#include "llae/app.h"

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

    void async_continue::on_async() {
        auto& l = llae::app::get(get_handle()->loop).lua();
        if (!l.native()) {
            release();
            remove_ref();
            return;
        }
        l.checkstack(2);
        m_cont.push(l);
        auto toth = l.tothread(-1);
        toth.checkstack(3);
        int nargs = on_cont(toth);
        auto s = toth.resume(l,nargs);
        if (s != lua::status::ok && s != lua::status::yield) {
            llae::app::show_error(toth,s);
        }
        l.pop(1);// thread
        reset(l);
        remove_ref();
    }

    int async_continue::send() {
        int r = async::send();
        if (r<0) {
            return r;
        }
        add_ref();
        return r;
    }
}
