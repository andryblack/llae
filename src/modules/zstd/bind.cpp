#include "lua/bind.h"

#include "uncompress.h"
#include "compress.h"

void archive::impl::ZSTD::pusherror(lua::state& l,ZSTD_ErrorCode z_err) {
    auto err_str = ZSTD_getErrorString(z_err);
    if (err_str) {
        l.pushstring(err_str);
    } else {
        l.pushstring("unknown");
    }
}

int luaopen_archive_zstd(lua_State* L) {
	lua::state l(L);
    
    lua::bind::object<archive::zstduncompress>::register_metatable(l,&archive::zstduncompress::lbind);
    lua::bind::object<archive::zstduncompress_read>::register_metatable(l,&archive::zstduncompress_read::lbind);
    lua::bind::object<archive::zstduncompress_to_stream>::register_metatable(l,&archive::zstduncompress_to_stream::lbind);
    
    l.createtable();
    lua::bind::object<archive::zstduncompress>::get_metatable(l);
    l.setfield(-2,"zstduncompress");
    lua::bind::object<archive::zstduncompress_read>::get_metatable(l);
    l.setfield(-2,"zstduncompress_read");
    lua::bind::object<archive::zstduncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"zstduncompress_to_stream");

    lua::bind::function(l,"new_zstd_read",&archive::zstduncompress_read::new_decompress);
    lua::bind::function(l,"new_zstd_to_stream",&archive::zstduncompress_to_stream::new_decompress);
    lua::bind::function(l,"decompress",&archive::zstduncompress::decompress);
    lua::bind::function(l,"compress",&archive::zstd_compress);
   
    return 1;
}
