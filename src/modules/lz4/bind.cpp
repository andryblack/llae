#include "uncompress.h"
#include "lua/bind.h"

int luaopen_archive_lz4(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<archive::lz4uncompress>::register_metatable(l,&archive::lz4uncompress::lbind);
    
    l.createtable();
    lua::bind::object<archive::lz4uncompress>::get_metatable(l);
    l.setfield(-2,"lz4uncompress");
   
    return 1;
}

