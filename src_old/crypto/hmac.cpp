#include "hmac.h"
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if (OPENSSL_VERSION_NUMBER<0x10100000L)
static HMAC_CTX* HMAC_CTX_new() {
	HMAC_CTX* res = new HMAC_CTX();
	HMAC_CTX_init(res);
	return res;
}
static void HMAC_CTX_free(HMAC_CTX* ctx) {
	HMAC_CTX_cleanup(ctx);
	delete ctx;
}
#endif

#include "../luabind.h"
#include <new>

static const char* mt_name = "llae.crypto.Hmac";


Hmac::Hmac() : m_ctx(HMAC_CTX_new()) {

}

Hmac::~Hmac() {
	HMAC_CTX_free(m_ctx);
}

int Hmac::update(lua_State* L) {
	Hmac* self = HmacRef::get_ptr(L,1);
	size_t len = 0;
	const unsigned char* data =  reinterpret_cast<const unsigned char*>(luaL_checklstring(L,2,&len));
	HMAC_Update(self->m_ctx,data,len);
	lua_pushvalue(L,1);
	return 1;
}

int Hmac::final(lua_State* L) {
	Hmac* self = HmacRef::get_ptr(L,1);
	luaL_Buffer buf;
	unsigned int size = EVP_MAX_MD_SIZE;
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,size));
	HMAC_Final(self->m_ctx,data,&size);
	luaL_pushresultsize(&buf,size);
	return 1;
}



int Hmac::lnew(lua_State* L) {
	const char* dname = luaL_checkstring(L,1);
	const EVP_MD* md = EVP_get_digestbyname(dname);
	if (!md) {
		luaL_error(L,"not found digest algo %s",dname);
	}
	size_t keylen = 0;
	const unsigned char* key = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,2,&keylen));
	HmacRef* ref = 
		new (lua_newuserdata(L,sizeof(HmacRef))) 
			HmacRef(new Hmac());
	HMAC_Init_ex(ref->get()->m_ctx,key,keylen,md,0);
	luaL_setmetatable(L,mt_name);
	return 1;
}

void Hmac::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"update",&Hmac::update);
	luabind::bind(L,"final",&Hmac::final);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&HmacRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}