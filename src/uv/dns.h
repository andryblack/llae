#ifndef __LLAE_UV_DNS_H_INCLUDED__
#define __LLAE_UV_DNS_H_INCLUDED__

#include "req.h"
#include "lua/ref.h"

namespace uv {

	class loop;

	class getaddrinfo_req : public req {
	private:
		uv_getaddrinfo_t m_req;
		lua::ref m_cont;
		static void getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
		void on_end(int status, struct addrinfo* res);
	public:
		explicit getaddrinfo_req(lua::ref&& cont);
		~getaddrinfo_req();
		uv_getaddrinfo_t* get() { return &m_req; }
		static lua::multiret getaddrinfo(lua::state& l);
	};

}

#endif /*__LLAE_UV_DNS_H_INCLUDED__*/