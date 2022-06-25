#include "socks5_tcp_connection.h"
#include "lua/bind.h"

int luaopen_net(lua_State* L) {
	lua::state l(L);

	lua::bind::object<net::socks5::tcp_connection>::register_metatable(l,&net::socks5::tcp_connection::lbind);

    l.createtable(); // net

	l.createtable();
	lua::bind::object<net::socks5::tcp_connection>::get_metatable(l);
	l.setfield(-2,"tcp_connection");

	l.setfield(-2,"socks5");

	return 1;
}
