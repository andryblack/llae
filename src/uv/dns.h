#ifndef __LLAE_UV_DNS_H_INCLUDED__
#define __LLAE_UV_DNS_H_INCLUDED__

#include "req.h"
#include "lua/ref.h"
#include "llae/promise.h"

namespace llae {
	class loop;
}

namespace uv {

	class loop;

	struct addrinfo_item {
		enum class family_t {
			ipv4,
			ipv6,
			unknown
		};
		family_t family = family_t::unknown;
		enum class socktype_t {
			any,
			tcp,
			udp,
			unknown
		};
		socktype_t socktype = socktype_t::any;
		std::string addr;
	};

	class getaddrinfo_req : public req {
	private:
		uv_getaddrinfo_t m_req;
		llae::result_promise_ptr<std::vector<addrinfo_item>> m_promise;
		static void getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
		void on_end(int status, struct addrinfo* res);
		explicit getaddrinfo_req(const llae::result_promise_ptr<std::vector<addrinfo_item>>& promise);

	public:
		~getaddrinfo_req();
		uv_getaddrinfo_t* get() { return &m_req; }
		static llae::result_promise_ptr<std::vector<addrinfo_item>> async_getaddrinfo(llae::loop& l,std::string_view host,std::optional<std::string_view> service);
	};

}

namespace lua {
	template<>
	struct stack<uv::addrinfo_item> {
		static int push(lua::state& l,const uv::addrinfo_item& item);
	};
	template<>
	struct stack<const uv::addrinfo_item&> : stack<uv::addrinfo_item> {};
	template<>
	struct stack<std::vector<uv::addrinfo_item>>  {
		static int push(lua::state& l,const std::vector<uv::addrinfo_item>& items);
	};
	template<>
	struct stack<const std::vector<uv::addrinfo_item>&> : stack<std::vector<uv::addrinfo_item>> {};
}

#endif /*__LLAE_UV_DNS_H_INCLUDED__*/