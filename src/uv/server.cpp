#include "server.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "llae/async_bind.h"

META_OBJECT_INFO(uv::server,uv::handle)


namespace uv {

	server::server() {
	}

	server::~server() {
	}

	void server::on_closed() {
        if (m_listen_promise) {
			auto result = std::move(m_listen_promise);
			result->set_result(llae::result<void>(llae::string_error::create("server closed")));
        }
		handle::on_closed();
	}

	void server::connection_cb(uv_stream_t* s, int status) {
		if (s->data) {
			static_cast<server*>(s->data)->on_connection(status);
		}
	}

	void server::on_connection(int st) {
		if (m_listen_promise) {
			auto result = std::move(m_listen_promise);
			if (st < 0) {
				result->set_result(status_error::create(st));
 			} else {
				result->set_result(llae::result<void>());
			}
		}
	}

	llae::result_promise_ptr<void> server::listen(lua::state& l) {
		if (m_listen_promise) {
			return llae::result_promise_forward_error<void>(llae::string_error::create("already listening"));
		}
		if (m_state == state_t::s_closing) {
			m_state = state_t::s_none;
			return llae::result_promise_forward_error<void>(llae::string_error::create("server closing"));
		}
        auto backlog = l.checkinteger(2);
		m_listen_promise = common::make_intrusive<llae::result_promise<void>>();
		if (m_state == state_t::s_none) {
			int res = uv_listen(get_stream(),int(backlog),&server::connection_cb);
			if (res < 0) {
				m_listen_promise.reset();
				return llae::result_promise_forward_error<void>(status_error::create(res));
			}
			m_state = state_t::s_listening;
		}
		return m_listen_promise;
	}

	llae::result<void> server::accept(lua::state& l,const stream_ptr& stream) {
        assert(stream);
        assert(!stream->is_closing());
        assert(!stream->is_closed());
		int res = uv_accept(get_stream(),stream->get_stream());
		return make_result(res);
	}

	void server::stop(lua::state& l) {
		if (m_state == state_t::s_closing) {
			return;
		}
		m_state = state_t::s_closing;
		if (m_listen_promise) {
			auto result = std::move(m_listen_promise);
			result->set_result(llae::result<void>(llae::string_error::create("server close")));
        }
		if (!is_closing()) {
			close();
		}
	}

	void server::lbind(lua::state& l) {
		llae::async_function(l,"listen",&server::listen);
		lua::bind::function(l,"accept",&server::accept);
		lua::bind::function(l,"stop",&server::stop);
	}
}
