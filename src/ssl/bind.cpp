#include "connection.h"
#include "ctx.h"
#include "lua/bind.h"

int luaopen_ssl(lua_State* L) {
	lua::state l(L);

	lua::bind::object<ssl::ctx>::register_metatable(l,&ssl::ctx::lbind);
	lua::bind::object<ssl::connection>::register_metatable(l,&ssl::connection::lbind);

	l.createtable();
	lua::bind::object<ssl::connection>::get_metatable(l);
	l.setfield(-2,"connection");

	lua::bind::object<ssl::ctx>::get_metatable(l);
	l.setfield(-2,"ctx");

	return 1;
}