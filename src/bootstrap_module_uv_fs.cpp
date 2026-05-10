#include "lua/bind.h"
#include "llae/async_bind.h"

#include "../.././src/uv/fs.h"



int luaopen_uv_fs(lua_State* L) {
    lua::state l(L);
    
    l.createtable();
    
    
    lua::bind::function(l,"mkdir",&uv::fs::mkdir);
    lua::bind::function(l,"rmdir",&uv::fs::rmdir);
    lua::bind::function(l,"unlink",&uv::fs::unlink);
    lua::bind::function(l,"copyfile",&uv::fs::copyfile);
    lua::bind::function(l,"rename",&uv::fs::rename);
    lua::bind::function(l,"stat",&uv::fs::stat);
    lua::bind::function(l,"scandir",&uv::fs::scandir);
    lua::bind::function(l,"open",&uv::fs::open);
    lua::bind::function(l,"chmod",&uv::fs::chmod);
    
    
    lua::bind::value(l, "O_RDONLY", uv::fs::LO_RDONLY);
    lua::bind::value(l, "O_RDWR", uv::fs::LO_RDWR);
    lua::bind::value(l, "O_WRONLY", uv::fs::LO_WRONLY);
    lua::bind::value(l, "O_CREAT", uv::fs::LO_CREAT);
    lua::bind::value(l, "O_APPEND", uv::fs::LO_APPEND);
    return 1;
}
