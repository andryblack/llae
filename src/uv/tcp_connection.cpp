#include "tcp_connection.h"
#include "llae/app.h"
#include "luv.h"
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

	llae::result_promise_ptr<void> tcp_connection::connect(llae::loop& /*l*/, std::string_view host, int port) {
		struct sockaddr_storage addr;
		if (uv_ip4_addr(host.data(), port, (struct sockaddr_in*)&addr) &&
	      	uv_ip6_addr(host.data(), port, (struct sockaddr_in6*)&addr)) {
			char msg[256];
			snprintf(msg, sizeof(msg), "invalid IP address or port [%.*s:%d]",
				int(host.size()), host.data(), port);
			return llae::make_result_promise_string_error<void>(msg);
	   	}

		auto promise = common::make_intrusive<llae::result_promise<void>>();
		common::intrusive_ptr<connect_req> req{new connect_req(tcp_connection_ptr(this),promise)};
		int r = req->connect((struct sockaddr *)&addr);
		if (r < 0) {
			promise->set_result(uv::status_error::create(r));
		}
		return promise;
	}

    lua::multiret tcp_connection::getpeername(lua::state& l) {
        struct sockaddr_storage addr;
        int len = sizeof(addr);
        int r = uv_tcp_getpeername(&m_tcp,(struct sockaddr*)&addr,&len);
        if (r < 0) {
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
        char name[128];
        if (uv_ip4_name((const struct sockaddr_in*)&addr,name,sizeof(name)) == 0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in*)&addr)->sin_port));
            return {2};
        } else if (uv_ip6_name((const struct sockaddr_in6*)&addr,name,sizeof(name))==0) {
            l.pushstring(name);
            l.pushinteger(ntohs(((const struct sockaddr_in6*)&addr)->sin6_port));
            return {2};
        }
        l.pushnil();
        l.pushstring("failed decode");
        return {2};
    }

	lua::multiret tcp_connection::keepalive(lua::state& l) {
		int enable = l.toboolean(2);
		int delay = 0;
		if (enable) {
			delay = int(l.checkinteger(3));
		}
		auto r = uv_tcp_keepalive(&m_tcp,enable,delay);
		return return_status_error(l,r);
	}

	lua::multiret tcp_connection::nodelay(lua::state& l) {
		int enable = l.toboolean(2);
		auto r = uv_tcp_nodelay(&m_tcp,enable);
		return return_status_error(l,r);
	}

}
