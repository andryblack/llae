#include "lua/bind.h"
#include "llae/async_bind.h"

#include "crypto/md.h"
#include "crypto/random.h"
#include "crypto/crypto.h"

/* class crypto::md */
static void luabind_crypto_md(lua::state& l) {
    
    
    llae::async_function(l,"update",&crypto::md::lasync_update);
    llae::async_function(l,"finish",&crypto::md::async_finish);
    lua::bind::function(l,"get_length",&crypto::md::get_length);
    lua::bind::function(l,"new",&crypto::md::lnew);
    
    
}

/* class crypto::entropy */
static void luabind_crypto_entropy(lua::state& l) {
    
    
    lua::bind::function(l,"update_manual",&crypto::entropy::update_manual);
    lua::bind::function(l,"new",&crypto::entropy::lnew);
    
    
}

/* class crypto::random */
static void luabind_crypto_random(lua::state& l) {
    
    
    lua::bind::function(l,"update",&crypto::random::update);
    lua::bind::function(l,"seed",&crypto::random::lseed);
    lua::bind::function(l,"new",&crypto::random::lnew);
    
    
}



int luaopen_crypto(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<crypto::md>::register_metatable(l, &luabind_crypto_md);
    
    
    lua::bind::object<crypto::entropy>::register_metatable(l, &luabind_crypto_entropy);
    
    lua::bind::object<crypto::random>::register_metatable(l, &luabind_crypto_random);
    
    
    l.createtable();
    
    
    lua::bind::object<crypto::md>::get_metatable(l);
    l.setfield(-2,"md");
    
    
    lua::bind::object<crypto::entropy>::get_metatable(l);
    l.setfield(-2,"entropy");
    
    lua::bind::object<crypto::random>::get_metatable(l);
    l.setfield(-2,"random");
    
    
    llae::async_function(l,"crc32",&crypto::async_crc32);
    
    
    lua::bind::value(l, "MD_NONE", crypto::MD_NONE);
    lua::bind::value(l, "MD_MD5", crypto::MD_MD5);
    lua::bind::value(l, "MD_SHA1", crypto::MD_SHA1);
    lua::bind::value(l, "MD_SHA224", crypto::MD_SHA224);
    lua::bind::value(l, "MD_SHA256", crypto::MD_SHA256);
    lua::bind::value(l, "MD_SHA384", crypto::MD_SHA384);
    lua::bind::value(l, "MD_SHA512", crypto::MD_SHA512);
    lua::bind::value(l, "MD_RIPEMD160", crypto::MD_RIPEMD160);
    return 1;
}
