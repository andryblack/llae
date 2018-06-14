#ifndef __LLAE_TCP_SERVER_H_INCLUDED__
#define __LLAE_TCP_SERVER_H_INCLUDED__

#include "llae.h"
#include "uv_handle_holder.h"
#include "lua_holder.h"
#include "tcp_connection.h"

class TCPServer : public UVHandleHolder {
private:
	uv_tcp_t m_tcp;
	uv_stream_t* get_stream();
	virtual uv_handle_t* get_handle();
protected:
	static void connection_cb(uv_stream_t* server, int status);
	void on_connection(int status);
	LuaFunction m_connection_cb;
public:
	explicit TCPServer(uv_loop_t* loop);
	static int lnew(lua_State* L);
	static void lbind(lua_State* L);
	void push(lua_State* L);
	void bind(lua_State* L,const char* addr,int port);
	void listen(lua_State* L,int backlog,const luabind::function& cb);
	void accept(lua_State* L,const TCPConnectionRef& con);
	void stop(lua_State* L);
};
typedef Ref<TCPServer> TCPServerRef;

#endif /*__LLAE_TCP_SERVER_H_INCLUDED__*/