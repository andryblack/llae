#include "tcp_server.h"
#include "llae/app.h"
#include "llae/result.h"
#include "luv.h"
#include "net/address_lua.h"
#include "common/intrusive_ptr.h"
#include "lua/stack.h"

META_OBJECT_INFO(uv::tcp_server,uv::server)


namespace uv {

	tcp_server::tcp_server(loop& l) {
		int r = uv_tcp_init(l.native(),&m_tcp);
		UV_DIAG_CHECK(r);
		attach();
	}
	tcp_server::~tcp_server() {
	}

	llae::result<void> tcp_server::bind(const net::address& address) {
		struct sockaddr_storage addr;
		auto res = address.to_sockaddr(addr);
		if (res.get_error()) {
			return res;
		}
		auto r = uv_tcp_bind(&m_tcp,(struct sockaddr*)&addr,0);
		return return_status(r);
	}

	llae::result<net::address> tcp_server::getsockname() const {
		struct sockaddr_storage addr;
		int len = sizeof(addr);
		int r = uv_tcp_getsockname(const_cast<uv_tcp_t*>(&m_tcp),(struct sockaddr*)&addr,&len);
		if (r < 0) {
			return uv::status_error::create(r);
		}
		return net::address::decode(reinterpret_cast<struct sockaddr*>(&addr));
	}

	lua::multiret tcp_server::lnew(lua::state& l) {
		common::intrusive_ptr<tcp_server> server{new tcp_server(llae::app::get(l).loop())};
		lua::push(l,std::move(server));
		return {1};
	}
}
