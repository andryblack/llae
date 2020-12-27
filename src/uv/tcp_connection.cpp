#include "tcp_connection.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <iostream>

META_OBJECT_INFO(uv::tcp_connection,uv::stream)


namespace uv {

	class connect_req : public req {
	private:
		uv_connect_t m_req;
		tcp_connection_ptr m_conn;
		lua::ref m_cont;
	public:
		connect_req(tcp_connection_ptr&& con,lua::ref&& cont) : m_conn(std::move(con)),m_cont(std::move(cont)) {
			attach(reinterpret_cast<uv_req_t*>(get()));
		}
		uv_connect_t* get() { return &m_req; }
		static void connect_cb(uv_connect_t* req, int status) {
			auto self = static_cast<connect_req*>(req->data);
			self->on_end(status);
			self->remove_ref();
		}
		void on_end(int status) {
            if (llae::app::closed(m_conn->get_handle()->loop)) {
                m_cont.release();
                return;
            }
			auto& l = llae::app::get(m_conn->get_handle()->loop).lua();
            if (!l.native()) {
                m_cont.release();
                return;
            }
            l.checkstack(2);
			m_cont.push(l);
			auto toth = l.tothread(-1);
			toth.checkstack(3);
			int nargs;
			if (status < 0) {
				toth.pushnil();
				uv::push_error(toth,status);
				nargs = 2;
			} else {
				l.pushboolean(true);
				nargs = 1;
			}
			auto s = toth.resume(l,nargs);
			if (s != lua::status::ok && s != lua::status::yield) {
				llae::app::show_error(toth,s);
			}
			l.pop(1);// thread
			m_cont.reset(l);
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

	lua::multiret tcp_connection::connect(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("tcp_connection::connect is async");
			return {2};
		}
		
		{
			const char* host = l.checkstring(2);
			int port = l.checkinteger(3);
			struct sockaddr_storage addr;
			if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
		      	uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
		    	l.error("invalid IP address or port [%s:%d]", host, port);
		   	}

			l.pushthread();
			lua::ref connect_cont;
			connect_cont.set(l);
		
			common::intrusive_ptr<connect_req> req{new connect_req(tcp_connection_ptr(this),
				std::move(connect_cont))};
			req->add_ref();
			int r = uv_tcp_connect(req->get(),&m_tcp,(struct sockaddr *)&addr,&connect_req::connect_cb);
			if (r < 0) {
				req->remove_ref();
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			}
		} 
		l.yield(0);
		return {0};
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

	void tcp_connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_connection::lnew);
		lua::bind::function(l,"connect",&tcp_connection::connect);
        lua::bind::function(l,"getpeername",&tcp_connection::getpeername);
	}
}
