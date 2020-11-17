#include "gzstream.h"
#include "lua/bind.h"

int luaopen_archive(lua_State* L) {
	lua::state l(L);

	lua::bind::object<archive::zlibcompress>::register_metatable(l,&archive::zlibcompress::lbind);
	lua::bind::object<archive::zlibcompress_read>::register_metatable(l,&archive::zlibcompress_read::lbind);
	lua::bind::object<archive::zlibcompress_to_stream>::register_metatable(l,&archive::zlibcompress_to_stream::lbind);
	
	l.createtable();
	lua::bind::object<archive::zlibcompress>::get_metatable(l);
	l.setfield(-2,"zcompress");
	lua::bind::object<archive::zlibcompress_read>::get_metatable(l);
	l.setfield(-2,"zcompress_read");
	lua::bind::object<archive::zlibcompress_to_stream>::get_metatable(l);
	l.setfield(-2,"zcompress_to_stream");

	lua::bind::function(l,"new_deflate_read",&archive::zlibcompress_read::new_deflate);
	lua::bind::function(l,"new_deflate_to_stream",&archive::zlibcompress_to_stream::new_deflate);


	return 1;
} 
