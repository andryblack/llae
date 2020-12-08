#include "zlibcompress.h"
#include "zlibuncompress.h"
#include "lua/bind.h"

int luaopen_archive(lua_State* L) {
	lua::state l(L);

	lua::bind::object<archive::zlibcompress>::register_metatable(l,&archive::zlibcompress::lbind);
	lua::bind::object<archive::zlibcompress_read>::register_metatable(l,&archive::zlibcompress_read::lbind);
	lua::bind::object<archive::zlibcompress_to_stream>::register_metatable(l,&archive::zlibcompress_to_stream::lbind);
    
    lua::bind::object<archive::zlibuncompress>::register_metatable(l,&archive::zlibuncompress::lbind);
    lua::bind::object<archive::zlibuncompress_read>::register_metatable(l,&archive::zlibuncompress_read::lbind);
    lua::bind::object<archive::zlibuncompress_to_stream>::register_metatable(l,&archive::zlibuncompress_to_stream::lbind);
	
	l.createtable();
	lua::bind::object<archive::zlibcompress>::get_metatable(l);
	l.setfield(-2,"zcompress");
	lua::bind::object<archive::zlibcompress_read>::get_metatable(l);
	l.setfield(-2,"zcompress_read");
	lua::bind::object<archive::zlibcompress_to_stream>::get_metatable(l);
	l.setfield(-2,"zcompress_to_stream");
    
    lua::bind::object<archive::zlibuncompress>::get_metatable(l);
    l.setfield(-2,"zuncompress");
    lua::bind::object<archive::zlibuncompress_read>::get_metatable(l);
    l.setfield(-2,"zuncompress_read");
    lua::bind::object<archive::zlibuncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"zuncompress_to_stream");

	lua::bind::function(l,"new_deflate_read",&archive::zlibcompress_read::new_deflate);
	lua::bind::function(l,"new_gzip_read",&archive::zlibcompress_read::new_gzip);
	lua::bind::function(l,"new_deflate_to_stream",&archive::zlibcompress_to_stream::new_deflate);
    
    lua::bind::function(l,"new_inflate_read",&archive::zlibuncompress_read::new_inflate);
    lua::bind::function(l,"new_gunzip_read",&archive::zlibuncompress_read::new_gzip);
    lua::bind::function(l,"new_inflate_to_stream",&archive::zlibuncompress_to_stream::new_inflate);


	return 1;
} 
