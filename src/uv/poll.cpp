#include "poll.h"

#include "llae/app.h"
#include "luv.h"


META_OBJECT_INFO(uv::poll,uv::handle)


namespace uv {

	

	poll::poll(loop& l,int fd) {
		int r = uv_poll_init(l.native(),&m_poll,fd);
		UV_DIAG_CHECK(r);
		attach();
	}
	poll::poll(loop& l,posix::fd_ptr && fd) : m_fd(std::move(fd)) {
		int r = uv_poll_init(l.native(),&m_poll,m_fd->get());
		UV_DIAG_CHECK(r);
		attach();
	}
	poll::~poll() {
        reject(llae::string_error::create("poll destroyed"));
	}

    void poll::resolve(int events) {
        auto p = std::move(m_poll_promise);
        if (p) {
            p->set_result(llae::result<int>(events));
        }
    }

    void poll::reject(llae::error_ptr&& error) {
        auto p = std::move(m_poll_promise);
        if (p) {
            p->set_result(llae::result<int>(std::move(error)));
        }
    }

    llae::result_promise_ptr<int> poll::poll_async(int events) {
        if (m_poll_promise) {
            return llae::result_promise_forward_error<int>(llae::string_error::create("already polling"));
        }
        m_poll_promise = common::make_intrusive<llae::result_promise<int>>();
        int res = uv_poll_start(&m_poll, events, &poll::on_poll_cb);
        if (res < 0) {
            m_poll_promise.reset();
            return llae::result_promise_forward_error<int>(status_error::create(res));
        }
        add_ref();
        return m_poll_promise;
    }

	llae::result<void> poll::stop_poll() {
        auto res = uv_poll_stop(&m_poll);
        reject(llae::string_error::create("stop"));
        return make_result(res);
	}

	void poll::on_poll(int status, int events) {
        if (status) {
            reject(status_error::create(status));
        } else {
            resolve(events);
        }
	}
	void poll::on_poll_cb(uv_poll_t *handle, int status, int events) {
		poll* self = static_cast<poll*>(handle->data);
        uv_poll_stop(handle);
        self->on_poll(status,events);
        self->remove_ref();
	}
	void poll::on_closed() {
		reject(llae::string_error::create("poll closed"));
        handle::on_closed();
	}

	poll_ptr poll::lnew(lua::state& l) {
		posix::fd_ptr fdp = lua::stack<posix::fd_ptr>::get(l,1);
		if (!fdp) {
			uv_file fd = uv_file(l.checkinteger(1));
			common::intrusive_ptr<poll> res{new poll(llae::app::get(l).loop(),fd)};
			return res;
		} else {
			common::intrusive_ptr<poll> res{new poll(llae::app::get(l).loop(),std::move(fdp))};
			return res;
		}
	}
		
}
