#include "uncompress.h"
#include "lua/bind.h"

int luaopen_archive_lz4(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<archive::lz4uncompress>::register_metatable(l,&archive::lz4uncompress::lbind);
    lua::bind::object<archive::lz4uncompress_read>::register_metatable(l,&archive::lz4uncompress_read::lbind);
    lua::bind::object<archive::lz4uncompress_to_stream>::register_metatable(l,&archive::lz4uncompress_to_stream::lbind);
    
    l.createtable();
    lua::bind::object<archive::lz4uncompress>::get_metatable(l);
    l.setfield(-2,"lz4uncompress");
    lua::bind::object<archive::lz4uncompress_read>::get_metatable(l);
    l.setfield(-2,"lz4uncompress_read");
    lua::bind::object<archive::lz4uncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"lz4uncompress_to_stream");

    lua::bind::function(l,"new_lz4_read",&archive::lz4uncompress_read::new_decompress);
    lua::bind::function(l,"new_lz4_to_stream",&archive::lz4uncompress_to_stream::new_decompress);

    return 1;
}

