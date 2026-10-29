#include "lua/bind.h"
#include "llae/async_bind.h"

#include "ssl/connection.h"
#include "ssl/ctx.h"


/* class ssl::connection */
static void luabind_ssl_connection(lua::state& l) {
    
    
    lua::bind::constructor<ssl::connection, ssl::ctx_ptr&&, uv::stream_ptr&&>(l);
    
    
    
    lua::bind::function(l,"configure",&ssl::connection::configure);
    lua::bind::function(l,"set_host",&ssl::connection::set_host);
    lua::bind::function(l,"handshake",&ssl::connection::handshake);
    lua::bind::function(l,"write",&ssl::connection::write);
    lua::bind::function(l,"read",&ssl::connection::read);
    lua::bind::function(l,"close",&ssl::connection::close);
    lua::bind::function(l,"shutdown",&ssl::connection::shutdown);
    lua::bind::function(l,"add_read_buffer",&ssl::connection::add_read_buffer);
    lua::bind::function(l,"start_read",&ssl::connection::start_read);
    lua::bind::function(l,"stop_read",&ssl::connection::stop_read);
    
    
    
}

/* class ssl::ctx */
static void luabind_ssl_ctx(lua::state& l) {
    
    
    lua::bind::constructor<ssl::ctx, crypto::random_ptr&&>(l);
    
    
    
    lua::bind::function(l,"init",&ssl::ctx::init);
    lua::bind::function(l,"load_cert",&ssl::ctx::load_cert);
    lua::bind::function(l,"load_system_certs",&ssl::ctx::load_system_certs);
    lua::bind::function(l,"set_debug_threshold",&ssl::ctx::set_debug_threshold);
    
    
    
    lua::bind::value(l, "default_cafile", ssl::ctx::default_cafile);
}


int luaopen_ssl(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<ssl::connection>::register_metatable(l, &luabind_ssl_connection);
    
    lua::bind::object<ssl::ctx>::register_metatable(l, &luabind_ssl_ctx);
    
    l.createtable();
    
    lua::bind::object<ssl::connection>::get_metatable(l);
    l.setfield(-2,"connection");
    
    lua::bind::object<ssl::ctx>::get_metatable(l);
    l.setfield(-2,"ctx");
    
    
    
    
    return 1;
}
