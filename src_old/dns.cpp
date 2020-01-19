#include "dns.h"
#include <iostream>

GetAddrInfoReq::GetAddrInfoReq() {
	attach();
}

uv_req_t* GetAddrInfoReq::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_req);
}

void GetAddrInfoReq::on_cb(int status, struct addrinfo* res) {
	
}

void GetAddrInfoReq::getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
	//std::cout << "GetAddrInfoReq::getaddrinfo_cb" << std::endl;
	GetAddrInfoReq* r = static_cast<GetAddrInfoReq*>(req->data);
	r->on_cb(status,res);
	uv_freeaddrinfo(res);
	r->remove_ref();
}

void GetAddrInfoLuaThreadReq::on_cb(int status, struct addrinfo* res) {
	uv_loop_t* loop = m_req.loop;
	lua_State* L = llae_get_vm(loop);
	if (L && m_cb) {
		const char* err = 0;
		if (status < 0) {
			err = uv_strerror(status);
		}
		on_result(L,res,err);
		m_cb.reset(L);
	}
}

static void push_addrinfo_i(lua_State* L,struct addrinfo* res) {
	lua_newtable(L);
	switch (res->ai_family) {
		case AF_INET:
			lua_pushstring(L,"ip4");
			break;
		case AF_INET6:
			lua_pushstring(L,"ip6");
			break;
		default:
			lua_pushstring(L,"unknown");
			break;
	};
	lua_setfield(L,-2,"family");
	switch (res->ai_socktype) {
		case SOCK_STREAM:
			lua_pushstring(L,"tcp");
			break;
		case SOCK_DGRAM:
			lua_pushstring(L,"udp");
			break;
		default:
			lua_pushstring(L,"unknown");
			break;
	};
	lua_setfield(L,-2,"socktype");

	switch (res->ai_family) {
		case AF_INET: {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
			char buf[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(addr_in->sin_addr), buf, INET_ADDRSTRLEN);
			lua_pushstring(L,buf);
			lua_setfield(L,-2,"addr");
		} break;
		case AF_INET6:
			struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)res->ai_addr;
			char buf[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(addr_in6->sin6_addr), buf, INET6_ADDRSTRLEN);
			lua_pushstring(L,buf);
			lua_setfield(L,-2,"addr");
			break;
	;}
}

static void push_addrinfo(lua_State* L,struct addrinfo* res) {
	lua_newtable(L);
	lua_Integer i = 0;
	while (res) {
		push_addrinfo_i(L,res);
		lua_seti(L,-2,++i);
		res = res->ai_next;
	}
}

GetAddrInfoLuaThreadReq::GetAddrInfoLuaThreadReq() {

}

void GetAddrInfoLuaThreadReq::on_result(lua_State* L,struct addrinfo* res,const char* err) {
	if (m_cb) {
		//std::cout << "GetAddrInfoLuaThreadReq::on_result " << std::endl;
		int nres = 0;
		if (res) {
			push_addrinfo(L,res);
			++nres;
		}
		if (err) {
			lua_pushstring(L,err);
			++nres;
		}
		m_cb.resumevi(L,"GetAddrInfoLuaThreadReq::on_result",nres);
	}
}


int GetAddrInfoLuaThreadReq::getaddrinfo(lua_State* L,const char* node, const char* service, const struct addrinfo* hints,
		const luabind::thread& f) {
	m_cb.assign(L,f);
	int r = uv_getaddrinfo(llae_get_loop(L),&m_req,&GetAddrInfoReq::getaddrinfo_cb,node,service,hints);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		add_ref();
	}
	return r;
}

int GetAddrInfoLuaThreadReq::lua_getaddrinfo(lua_State *L) {
	const char* host = luaL_checkstring(L,1);
	struct addrinfo hint;
	memset(&hint,0,sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
	if (lua_gettop(L)>1 && lua_istable(L,1)) {

	}
    if (lua_isyieldable(L)) {
        // async thread
        lua_pushthread(L);
        luabind::thread th = luabind::S<luabind::thread>::get(L,-1);
        GetAddrInfoLuaThreadReqRef req(new GetAddrInfoLuaThreadReq());
        int res = req->getaddrinfo(L,host,0,&hint,th);
        lua_llae_handle_error(L,"llae.getaddrinfo",res);
        lua_yield(L,0);
    } else {
        luaL_error(L,"sync llae.getaddrinfo not supported");
    }
    return 0;
}

