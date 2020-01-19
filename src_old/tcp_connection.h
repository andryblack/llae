#ifndef __LLAE_TCP_CONNECTION_H_INCLUDED__
#define __LLAE_TCP_CONNECTION_H_INCLUDED__

#include "stream.h"
#include "uv_req_holder.h"

class ConnectReq : public UVReqHolder {
protected:
	uv_connect_t m_req;
	virtual void on_cb(int status);
	static void connect_cb(uv_connect_t* req,int status);
	ConnectReq();
	virtual uv_req_t* get_req();	
public:
	uv_connect_t* get_connect_req() { return &m_req; }
};

class TCPConnection;

class ConnectLuaThreadReq : public ConnectReq {
protected:
	LuaThread m_cb;
	virtual void on_cb(int status);
	virtual void on_result(lua_State* L,const char* err);
public:
	int connect(lua_State* L,const char* addr,int port,TCPConnection* con,const luabind::thread& f);
};
typedef Ref<ConnectLuaThreadReq> ConnectLuaThreadReqRef;


class TCPConnection : public Stream {
private:
	uv_tcp_t m_tcp;
protected:
public:
	explicit TCPConnection(uv_loop_t* loop);
	virtual uv_stream_t* get_stream();
	virtual void push(lua_State* L);
	void connect(lua_State* L,const char* addr,int port);
	uv_tcp_t* get_tcp() { return &m_tcp; }
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);
};
typedef Ref<TCPConnection> TCPConnectionRef;

#endif /*__LLAE_TCP_CONNECTION_H_INCLUDED__*/