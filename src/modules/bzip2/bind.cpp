#include "uncompress.h"
#include "lua/bind.h"

int luaopen_archive_bzip2(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<archive::bzlibuncompress>::register_metatable(l,&archive::bzlibuncompress::lbind);
    lua::bind::object<archive::bzlibuncompress_read>::register_metatable(l,&archive::bzlibuncompress_read::lbind);
    lua::bind::object<archive::bzlibuncompress_to_stream>::register_metatable(l,&archive::bzlibuncompress_to_stream::lbind);
    
    l.createtable();
    lua::bind::object<archive::bzlibuncompress>::get_metatable(l);
    l.setfield(-2,"bzuncompress");
    lua::bind::object<archive::bzlibuncompress_read>::get_metatable(l);
    l.setfield(-2,"bzuncompress_read");
    lua::bind::object<archive::bzlibuncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"bzuncompress_to_stream");

    lua::bind::function(l,"new_bz_read",&archive::bzlibuncompress_read::new_decompress);
    lua::bind::function(l,"new_bz_to_stream",&archive::bzlibuncompress_to_stream::new_decompress);

    
    return 1;
}

