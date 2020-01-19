#include "tcp_server.h"
#include "luabind.h"
#include <new>
#include <cassert>

static const char* mt_name = "llae.TCPServer";

TCPServer::TCPServer(uv_loop_t* loop)  {
	uv_tcp_init(loop,&m_tcp);
	attach();
}

uv_stream_t* TCPServer::get_stream() { 
	return reinterpret_cast<uv_stream_t*>(&m_tcp); 
}
uv_handle_t* TCPServer::get_handle() { 
	return reinterpret_cast<uv_handle_t*>(&m_tcp); 
}

void TCPServer::bind(lua_State* L,const char* host,int port) {
	struct sockaddr_storage addr;
	if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      	uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    	luaL_error(L, "invalid IP address or port [%s:%d]", host, port);
   	}
	int res = uv_tcp_bind(&m_tcp,(struct sockaddr*)&addr,0);
	lua_llae_handle_error(L,"TCPServer::bind",res);
}


void TCPServer::connection_cb(uv_stream_t* server, int status) {
	assert(server->data);
	static_cast<TCPServer*>(server->data)->on_connection(status);
}

void TCPServer::on_connection(int status) {
	if (m_connection_cb) {
		lua_State* L = llae_get_vm(get_handle());
		if (L) {
			const char* err = 0;
			if (status < 0) 
				err = uv_strerror(status);
			m_connection_cb.callv(L,"Sream::on_connection",err);
		}
	}
}

void TCPServer::listen(lua_State* L,int backlog,const luabind::function& cb) {
	m_connection_cb.assign(L,cb);
	int res = uv_listen(get_stream(),backlog,&TCPServer::connection_cb);
	lua_llae_handle_error(L,"TCPServer::listen",res);
}

void TCPServer::accept(lua_State* L,const TCPConnectionRef& con) {
	int res = uv_accept(get_stream(),con->get_stream());
	lua_llae_handle_error(L,"TCPServer::accept",res);
}


int TCPServer::lnew(lua_State* L) {
	(new TCPServer(llae_get_loop(L)))->push(L);
	return 1;
}

void TCPServer::push(lua_State* L) {
	new (lua_newuserdata(L,sizeof(TCPServerRef))) 
			TCPServerRef(this);
	UVHandleHolder::push(L);
	luaL_setmetatable(L,mt_name);
}

void TCPServer::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"bind",&TCPServer::bind);
	luabind::bind(L,"listen",&TCPServer::listen);
	luabind::bind(L,"accept",&TCPServer::accept);
	luabind::bind(L,"close",&UVHandleHolder::close);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&UVHandleHolder::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}
