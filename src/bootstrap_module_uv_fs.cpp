#include "lua/bind.h"
#include "llae/async_bind.h"

#include "../.././src/uv/fs.h"



int luaopen_uv_fs(lua_State* L) {
    lua::state l(L);
    
    l.createtable();
    
    
    llae::async_function(l,"mkdir",&uv::fs::async_mkdir);
    llae::async_function(l,"rmdir",&uv::fs::async_rmdir);
    llae::async_function(l,"unlink",&uv::fs::async_unlink);
    llae::async_function(l,"copyfile",&uv::fs::async_copyfile);
    llae::async_function(l,"rename",&uv::fs::async_rename);
    llae::async_function(l,"stat",&uv::fs::async_stat);
    llae::async_function(l,"scandir",&uv::fs::async_scandir);
    llae::async_function(l,"open",&uv::fs::async_open);
    llae::async_function(l,"chmod",&uv::fs::async_chmod);
    
    
    lua::bind::value(l, "O_RDONLY", uv::fs::LO_RDONLY);
    lua::bind::value(l, "O_RDWR", uv::fs::LO_RDWR);
    lua::bind::value(l, "O_WRONLY", uv::fs::LO_WRONLY);
    lua::bind::value(l, "O_CREAT", uv::fs::LO_CREAT);
    lua::bind::value(l, "O_APPEND", uv::fs::LO_APPEND);
    return 1;
}
