#include "dns.h"
#include "llae/app.h"
#include "luv.h"

namespace uv {


	static void push_addrinfo_i(lua::state& l,struct addrinfo* res) {
		l.createtable();
		switch (res->ai_family) {
			case AF_INET:
				l.pushstring("ip4");
				break;
			case AF_INET6:
				l.pushstring("ip6");
				break;
			default:
				l.pushstring("unknown");
				break;
		};
		l.setfield(-2,"family");
		switch (res->ai_socktype) {
			case SOCK_STREAM:
				l.pushstring("tcp");
				break;
			case SOCK_DGRAM:
				l.pushstring("udp");
				break;
			default:
				l.pushstring("unknown");
				break;
		};
		l.setfield(-2,"socktype");

		switch (res->ai_family) {
			case AF_INET: {
				struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
				char buf[INET_ADDRSTRLEN];
				uv_inet_ntop(AF_INET, &(addr_in->sin_addr), buf, INET_ADDRSTRLEN);
				l.pushstring(buf);
				l.setfield(-2,"addr");
			} break;
			case AF_INET6: {
				struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)res->ai_addr;
				char buf[INET6_ADDRSTRLEN];
				uv_inet_ntop(AF_INET6, &(addr_in6->sin6_addr), buf, INET6_ADDRSTRLEN);
				l.pushstring(buf);
				l.setfield(-2,"addr");
			} break;
		}
	}

	static void push_addrinfo(lua::state& l,struct addrinfo* res) {
		l.createtable();
		lua_Integer i = 0;
		while (res) {
			push_addrinfo_i(l,res);
			l.seti(-2,++i);
			res = res->ai_next;
		}
	}

	getaddrinfo_req::getaddrinfo_req(lua::ref&& cont) : m_cont(std::move(cont)) {
		attach(reinterpret_cast<uv_req_t*>(get()));
	}

	getaddrinfo_req::~getaddrinfo_req() {

	}

	void getaddrinfo_req::on_end(int status,struct addrinfo* res) {
		auto& l = llae::app::get(get()->loop).lua();
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
			push_addrinfo(toth,res);
			nargs = 1;
		}
		auto s = toth.resume(l,nargs);
		if (s != lua::status::ok && s != lua::status::yield) {
			llae::app::show_error(toth,s);
		}
		l.pop(1);// thread
		m_cont.reset(l);
	}

	void getaddrinfo_req::getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
		auto self = static_cast<getaddrinfo_req*>(req->data);
		self->on_end(status,res);
		if (res) {
			uv_freeaddrinfo(res);
		}
		self->remove_ref();
	}

	lua::multiret getaddrinfo_req::getaddrinfo(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("getaddrinfo is async");
			return {2};
		}
		{
			const char* node = l.checkstring(1);
			llae::app& app(llae::app::get(l));
			lua::ref cont;
			l.pushthread();
			cont.set(l);
			common::intrusive_ptr<getaddrinfo_req> req{new getaddrinfo_req(std::move(cont))};
			req->add_ref();
			int r = uv_getaddrinfo(app.loop().native(),
				req->get(),&getaddrinfo_req::getaddrinfo_cb,node,nullptr,nullptr);
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

}