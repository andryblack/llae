#ifndef __LLAE_DNS_H_INCLUDED__
#define __LLAE_DNS_H_INCLUDED__

#include <uv.h>
#include "uv_req_holder.h"
#include "lua_holder.h"
#include "luabind.h"

class GetAddrInfoReq : public UVReqHolder {
protected:
	uv_getaddrinfo_t m_req;
	virtual void on_cb(int status, struct addrinfo* res);
	static void getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
	GetAddrInfoReq();
	virtual uv_req_t* get_req();	
public:
	uv_getaddrinfo_t* get_getaddrinfo_req() { return &m_req; }
};

class GetAddrInfoLuaThreadReq : public GetAddrInfoReq {
protected:
	LuaThread m_cb;
	virtual void on_cb(int status, struct addrinfo* res);
	virtual void on_result(lua_State* L,struct addrinfo* res,const char* err);
public:
	GetAddrInfoLuaThreadReq();
	int getaddrinfo(lua_State* L,const char* node, const char* service, const struct addrinfo* hints,
		const luabind::thread& f);
	static int lua_getaddrinfo(lua_State *L);
};
typedef Ref<GetAddrInfoLuaThreadReq> GetAddrInfoLuaThreadReqRef;

#endif /*__LLAE_DNS_H_INCLUDED__*/