#include "tcp_connection.h"
#include <new>

static const char* mt_name = "llae.TCPConnection";


ConnectReq::ConnectReq() {
	attach();
}

uv_req_t* ConnectReq::get_req() {
	return reinterpret_cast<uv_req_t*>(&m_req);	
}

void ConnectReq::on_cb(int status) {

}

void ConnectReq::connect_cb(uv_connect_t* req,int status) {
	ConnectReq* r = static_cast<ConnectReq*>(req->data);
	r->on_cb(status);
	if (req->handle) {
		TCPConnection* con = static_cast<TCPConnection*>(req->handle->data);
		if (con) {
			con->remove_ref();
		}
	}
	r->remove_ref();
}


void ConnectLuaThreadReq::on_cb(int status) {
	uv_stream_t* stream = m_req.handle;
	uv_loop_t* loop = stream->loop;
	lua_State* L = llae_get_vm(loop);
	if (L && m_cb) {
		const char* err = 0;
		if (status < 0) {
			err = uv_strerror(status);
		}
		on_result(L,err);
		m_cb.reset(L);
	}
}

void ConnectLuaThreadReq::on_result(lua_State* L,const char* err) {
	if (m_cb) {
		m_cb.resumev(L,"ConnectLuaThreadReq::on_result",err==0,err);
	}
}

int ConnectLuaThreadReq::connect(lua_State* L,
	const char* addr,int port,
	TCPConnection* con,const luabind::thread& f) {

	union {
		struct sockaddr_in v4;
		struct sockaddr_in6 v6;
	} saddr;
	if (strchr(addr,':')) {
		if (uv_ip6_addr(addr,port,&saddr.v6) != 0) {
			luaL_error(L,"failed parse addr");
		}
	} else {
		if (uv_ip4_addr(addr,port,&saddr.v4) != 0) {
			luaL_error(L,"failed parse addr");
		}
	}
	
	m_cb.assign(L,f);
	int r = uv_tcp_connect(&m_req,con->get_tcp(), (struct sockaddr *) &saddr, ConnectReq::connect_cb);
	if (r < 0) {
		m_cb.reset(L);
	} else {
		con->add_ref();
		add_ref();
	}
	return r;
}


TCPConnection::TCPConnection(uv_loop_t* loop) : Stream() {
	uv_tcp_init(loop,&m_tcp);
	attach();
}

uv_stream_t* TCPConnection::get_stream() { 
	return reinterpret_cast<uv_stream_t*>(&m_tcp); 
}

int TCPConnection::lnew(lua_State* L) {
	(new TCPConnection(llae_get_loop(L)))->push(L);
	return 1;
}

void TCPConnection::push(lua_State* L) {
	new (lua_newuserdata(L,sizeof(TCPConnectionRef))) 
			TCPConnectionRef(this);
	UVHandleHolder::push(L);
	luaL_setmetatable(L,mt_name);
}

void TCPConnection::connect(lua_State* L,const char* addr,int port) {
	const char* path = luaL_checkstring(L,2);
	if (lua_isyieldable(L)) {
		// async thread
		lua_pushthread(L);
		luabind::thread th = luabind::S<luabind::thread>::get(L,-1);
		ConnectLuaThreadReqRef req(new ConnectLuaThreadReq());
		int res = req->connect(L,addr,port,this,th);
		lua_llae_handle_error(L,"TCPConnection::connect",res);
		lua_yield(L,0);
	} else {
		luaL_error(L,"sync TCPConnection::connect not supported");
	}
}

void TCPConnection::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"connect",&TCPConnection::connect);
	luaL_getmetatable(L,Stream::get_mt_name());
	lua_setmetatable(L,-2);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&UVHandleHolder::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}

