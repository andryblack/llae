#include "dns.h"
#include "llae/app.h"
#include "luv.h"

namespace lua {

	int stack<uv::addrinfo_item>::push(lua::state& l,const uv::addrinfo_item& item) {
		l.newtable();
		switch (item.family) {
			case uv::addrinfo_item::family_t::ipv4:
				l.pushstring("ip4");
				break;
			case uv::addrinfo_item::family_t::ipv6:
				l.pushstring("ip6");
				break;
			default:
				l.pushstring("unknown");
				break;
		}
		l.setfield(-2,"family");
		switch (item.socktype) {
			case uv::addrinfo_item::socktype_t::any:
				l.pushstring("any");
				break;
			case uv::addrinfo_item::socktype_t::tcp:
				l.pushstring("tcp");
				break;
			case uv::addrinfo_item::socktype_t::udp:
				l.pushstring("udp");
				break;
			default:
				l.pushstring("unknown");
				break;
		}
		l.setfield(-2,"socktype");
		l.pushstring(item.addr.c_str());
		l.setfield(-2,"addr");
		return 1;
	}

	int stack<std::vector<uv::addrinfo_item>>::push(lua::state& l,const std::vector<uv::addrinfo_item>& items) {
		l.newtable();
		lua_Integer idx = 1;
		for (const auto& item : items) {
			lua::stack<uv::addrinfo_item>::push(l,item);
			l.seti(-2,idx);
			++idx;
		}
		return 1;
	}
}

namespace uv {



	getaddrinfo_req::getaddrinfo_req(const llae::result_promise_ptr<std::vector<addrinfo_item>>& promise) : m_promise(promise) {
		attach(reinterpret_cast<uv_req_t*>(get()));
	}

	getaddrinfo_req::~getaddrinfo_req() {

	}

	void getaddrinfo_req::getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
		auto self = static_cast<getaddrinfo_req*>(req->data);
		self->on_end(status,res);
		self->remove_ref();
	}

	void getaddrinfo_req::on_end(int status,struct addrinfo* res) {
		if (!m_promise) {
			return;
		}
		if (status < 0) {
			m_promise->set_result(uv::status_error::create(status));
			return;
		}
		std::vector<addrinfo_item> items;
		struct addrinfo* ai = res;
		while (ai) {
			addrinfo_item item;
			if (ai->ai_family == AF_INET) {
				item.family = addrinfo_item::family_t::ipv4;
			} else if (ai->ai_family == AF_INET6) {
				item.family = addrinfo_item::family_t::ipv6;
			} 
			if (ai->ai_socktype == SOCK_STREAM) {
				item.socktype = addrinfo_item::socktype_t::tcp;
			} else if (ai->ai_socktype == SOCK_DGRAM) {
				item.socktype = addrinfo_item::socktype_t::udp;
			} else if (ai->ai_socktype == 0) {
				item.socktype = addrinfo_item::socktype_t::any;
			}
			if (ai->ai_addr->sa_family == AF_INET) {
				struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
				char buf[INET_ADDRSTRLEN];
				uv_inet_ntop(AF_INET, &(addr_in->sin_addr), buf, INET_ADDRSTRLEN);
				item.addr = buf;
			} else if (ai->ai_addr->sa_family == AF_INET6) {
				struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)res->ai_addr;
				char buf[INET6_ADDRSTRLEN];
				uv_inet_ntop(AF_INET6, &(addr_in6->sin6_addr), buf, INET6_ADDRSTRLEN);
				item.addr = buf;
			}
			items.emplace_back(std::move(item));
			ai = ai->ai_next;
		}
		m_promise->set_result(std::move(items));
	}

	llae::result_promise_ptr<std::vector<addrinfo_item>> getaddrinfo_req::async_getaddrinfo(llae::loop& l,std::string_view host,std::optional<std::string_view> service) {
		auto promise = llae::result_promise_ptr<std::vector<addrinfo_item>>(new llae::result_promise<std::vector<addrinfo_item>>());
		common::intrusive_ptr<getaddrinfo_req> req(new getaddrinfo_req(promise));
		const char* serv = service.has_value() ? service.value().data() : nullptr;
		int r = uv_getaddrinfo(get_native(l),req->get(),&getaddrinfo_req::getaddrinfo_cb,host.data(),serv,nullptr);
		if (r < 0) {
			promise->set_result(uv::status_error::create(r));
		} else {
			req->add_ref();
		}
		return promise;
	}


}