#include "lua/bind.h"
#include "llae/async_bind.h"

#include "../.././src/uv/os.h"



int luaopen_uv_os(lua_State* L) {
    lua::state l(L);
    
    l.createtable();
    
    
    lua::bind::function(l,"homedir",&uv::os::homedir);
    lua::bind::function(l,"tmpdir",&uv::os::tmpdir);
    lua::bind::function(l,"getenv",&uv::os::getenv);
    lua::bind::function(l,"setenv",&uv::os::setenv);
    lua::bind::function(l,"getallenv",&uv::os::getallenv);
    lua::bind::function(l,"unsetenv",&uv::os::unsetenv);
    lua::bind::function(l,"gethostname",&uv::os::gethostname);
    lua::bind::function(l,"uname",&uv::os::uname);
    lua::bind::function(l,"getpriority",&uv::os::getpriority);
    lua::bind::function(l,"setpriority",&uv::os::setpriority);
    lua::bind::function(l,"getpid",&uv::os::getpid);
    
    
    return 1;
}
