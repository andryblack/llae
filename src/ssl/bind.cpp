#include "connection.h"
#include "lua/bind.h"

int luaopen_ssl(lua_State* L) {
	lua::state l(L);

	lua::bind::object<ssl::connection>::register_metatable(l,&ssl::connection::lbind);

	l.createtable();
	lua::bind::object<ssl::connection>::get_metatable(l);
	l.setfield(-2,"connection");

	return 1;
}