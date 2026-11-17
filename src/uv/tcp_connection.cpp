#include "tcp_connection.h"
#include "llae/app.h"
#include "luv.h"
#include "net/address_lua.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include <cstdio>

META_OBJECT_INFO(uv::tcp_connection,uv::stream)


namespace uv {

	class tcp_connection::connect_req : public req {
	private:
		uv_connect_t m_req;
		tcp_connection_ptr m_conn;
		llae::result_promise_ptr<void> m_promise;
	public:
		connect_req(tcp_connection_ptr&& con,const llae::result_promise_ptr<void>& promise)
			: m_conn(std::move(con)),m_promise(promise) {
			attach(reinterpret_cast<uv_req_t*>(get()));
		}
		uv_connect_t* get() { return &m_req; }
		static void connect_cb(uv_connect_t* req, int status) {
			auto self = static_cast<connect_req*>(req->data);
			self->on_end(status);
			self->remove_ref();
		}
        int connect(struct sockaddr * addr) {
            add_ref();
            int r = uv_tcp_connect(&m_req,m_conn->get_tcp(),addr,&connect_req::connect_cb);
            if (r < 0) {
                remove_ref();
            }
            return r;
        }
		void on_end(int status) {
			if (!m_promise) {
				return;
			}
			if (status < 0) {
				m_promise->set_result(uv::status_error::create(status));
			} else {
				m_promise->set_result(llae::result<void>());
			}
		}
	};

	tcp_connection::tcp_connection(uv::loop& loop) {
		int r = uv_tcp_init(loop.native(),&m_tcp);
		UV_DIAG_CHECK(r);
		attach();
        //std::cout << "tcp_connection::tcp_connection" << std::endl;
	}
	tcp_connection::~tcp_connection() {
        //std::cout << "tcp_connection::~tcp_connection" << std::endl;
	}

	lua::multiret tcp_connection::lnew(lua::state& l) {
		common::intrusive_ptr<tcp_connection> connection{new tcp_connection(llae::app::get(l).loop())};
		lua::push(l,std::move(connection));
		return {1};
	}

	llae::result_promise_ptr<void> tcp_connection::connect(llae::loop& /*l*/, const net::address& address) {
		if (address.port == 0) {
			return llae::make_result_promise_string_error<void>("tcp_connection::connect port is required");
		}
		struct sockaddr_storage addr;
		auto res = address.to_sockaddr(addr);
		if (res.get_error()) {
			return llae::make_result_promise_string_error<void>(res.get_error()->to_string());
		}

		auto promise = common::make_intrusive<llae::result_promise<void>>();
		common::intrusive_ptr<connect_req> req{new connect_req(tcp_connection_ptr(this),promise)};
		int r = req->connect((struct sockaddr *)&addr);
		if (r < 0) {
			promise->set_result(uv::status_error::create(r));
		}
		return promise;
	}

	llae::result<net::address> tcp_connection::getpeername() const {
		struct sockaddr_storage addr;
		int len = sizeof(addr);
		int r = uv_tcp_getpeername(const_cast<uv_tcp_t*>(&m_tcp),(struct sockaddr*)&addr,&len);
		if (r < 0) {
			return uv::status_error::create(r);
		}
		return net::address::decode(reinterpret_cast<struct sockaddr*>(&addr));
	}

	llae::result<net::address> tcp_connection::getsockname() const {
		struct sockaddr_storage addr;
		int len = sizeof(addr);
		int r = uv_tcp_getsockname(const_cast<uv_tcp_t*>(&m_tcp),(struct sockaddr*)&addr,&len);
		if (r < 0) {
			return uv::status_error::create(r);
		}
		return net::address::decode(reinterpret_cast<struct sockaddr*>(&addr));
	}

	llae::result<> tcp_connection::keepalive(std::optional<int> delay) {
		int enable = delay.has_value();
		int delay_value = 0;
		if (enable) {
			delay_value = delay.value();
		}
		auto r = uv_tcp_keepalive(&m_tcp,enable,delay_value);
		return return_status(r);
	}

	llae::result<> tcp_connection::nodelay(bool enable) {
		auto r = uv_tcp_nodelay(&m_tcp,enable ? 1 : 0);
		return return_status(r);
	}

}
