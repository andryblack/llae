#include "llae.h"
#include <openssl/rand.h>
#include "crypto/hash.h"
#include "crypto/bignum.h"
#include "crypto/hmac.h"
#include "crypto/decipher.h"

static int lua_random_bytes(lua_State* L) {
    size_t size = luaL_checkinteger(L,1);
    luaL_Buffer buf;
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,size));
    RAND_bytes(data,size);
    luaL_pushresultsize(&buf,size);
    return 1;
}

static int lua_xor_bytes(lua_State* L) {
    size_t len1 = 0;
    size_t len2 = 0;
    const unsigned char* a1 = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,1,&len1));
    const unsigned char* a2 = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,2,&len2));
    if (len1!=len2) {
        luaL_error(L,"different lengths");
    }
    luaL_Buffer buf;
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,len1));
    for (size_t i=0;i<len1;++i) {
        data[i]=a1[i]^a2[i];
    }
    luaL_pushresultsize(&buf,len1);
    return 1;
}


extern "C" int luaopen_llae_crypto(lua_State* L) {
    OpenSSL_add_all_algorithms();
    
    Hash::lbind(L);
    Hmac::lbind(L);
    Bignum::lbind(L);
    Decipher::lbind(L);
	luaL_Reg reg[] = {
        { "random_bytes", lua_random_bytes },
        { "xor_bytes", lua_xor_bytes },
        { "createHash", Hash::lnew },
        { "createHmac", Hmac::lnew },
        { "createBignum", Bignum::lnew },
        { "createDecipher", Decipher::lnew },
        { NULL, NULL }
    };
    lua_newtable(L);
    luaL_setfuncs(L, reg, 0);
    return 1;
}