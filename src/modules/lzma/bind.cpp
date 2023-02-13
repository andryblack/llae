#include "lua/bind.h"

#include "uncompress.h"

void archive::impl::LZMA::pusherror(lua::state& l,stream& z,int z_err) {
    if(z_err == LZMA_DATA_ERROR) {
        l.pushstring("LZMA_DATA_ERROR");
    } else if(z_err == LZMA_MEM_ERROR) {
        l.pushstring("LZMA_MEM_ERROR");
    } else {
        l.pushstring("unknown");
    }
}

int luaopen_archive_lzma(lua_State* L) {
	lua::state l(L);
    
    lua::bind::object<archive::lzmauncompress>::register_metatable(l,&archive::lzmauncompress::lbind);
    lua::bind::object<archive::lzmauncompress_read>::register_metatable(l,&archive::lzmauncompress_read::lbind);
    lua::bind::object<archive::lzmauncompress_to_stream>::register_metatable(l,&archive::lzmauncompress_to_stream::lbind);
    
    l.createtable();
    lua::bind::object<archive::lzmauncompress>::get_metatable(l);
    l.setfield(-2,"lzmauncompress");
    lua::bind::object<archive::lzmauncompress_read>::get_metatable(l);
    l.setfield(-2,"lzmauncompress_read");
    lua::bind::object<archive::lzmauncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"lzmauncompress_to_stream");

    lua::bind::function(l,"new_lzma_read",&archive::lzmauncompress_read::new_decompress);
    lua::bind::function(l,"new_lzma_to_stream",&archive::lzmauncompress_to_stream::new_decompress);
    lua::bind::function(l,"decompress",&archive::lzmauncompress::decompress);

    return 1;
}