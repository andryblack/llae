#include "tcp_connection.h"
#include "llae/app.h"
#include "luv.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <iostream>

META_OBJECT_INFO(uv::tcp_connection,uv::stream)


namespace uv {

	class tcp_connection::connect_req : public req {
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
        int connect(struct sockaddr * addr) {
            add_ref();
            int r = uv_tcp_connect(&m_req,m_conn->get_tcp(),addr,&connect_req::connect_cb);
            if (r < 0) {
                remove_ref();
            }
            return r;
        }
        void reset(lua::state& l) {
            m_cont.reset(l);
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
                toth.pushboolean(true);
				nargs = 1;
			}
			auto s = toth.resume(toth,nargs);
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
            int r = req->connect((struct sockaddr *)&addr);
			if (r < 0) {
                req->reset(l);
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

	lua::multiret tcp_connection::keepalive(lua::state& l) {
		int enable = l.toboolean(2);
		int delay = 0;
		if (enable) {
			delay = l.checkinteger(3);
		}
		auto r = uv_tcp_keepalive(&m_tcp,enable,delay);
		return return_status_error(l,r);
	}

	lua::multiret tcp_connection::nodelay(lua::state& l) {
		int enable = l.toboolean(2);
		auto r = uv_tcp_nodelay(&m_tcp,enable);
		return return_status_error(l,r);
	}

	void tcp_connection::lbind(lua::state& l) {
		lua::bind::function(l,"new",&tcp_connection::lnew);
		lua::bind::function(l,"connect",&tcp_connection::connect);
        lua::bind::function(l,"getpeername",&tcp_connection::getpeername);
        lua::bind::function(l,"keepalive",&tcp_connection::keepalive);
        lua::bind::function(l,"nodelay",&tcp_connection::nodelay);
	}
}
