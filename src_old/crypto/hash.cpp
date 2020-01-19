#include "hash.h"
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if (OPENSSL_VERSION_NUMBER<0x10100000L)
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

#include "../luabind.h"
#include <new>

static const char* mt_name = "llae.crypto.Hash";


Hash::Hash() : m_ctx(EVP_MD_CTX_new()) {

}

Hash::~Hash() {
	EVP_MD_CTX_free(m_ctx);
}

int Hash::update(lua_State* L) {
	Hash* self = HashRef::get_ptr(L,1);
	size_t len = 0;
	const char* data = luaL_checklstring(L,2,&len);
	EVP_DigestUpdate(self->m_ctx,data,len);
	lua_pushvalue(L,1);
	return 1;
}

int Hash::final(lua_State* L) {
	Hash* self = HashRef::get_ptr(L,1);
	luaL_Buffer buf;
	unsigned int size = EVP_MAX_MD_SIZE;
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,size));
	EVP_DigestFinal(self->m_ctx,data,&size);
	luaL_pushresultsize(&buf,size);
	return 1;
}



int Hash::lnew(lua_State* L) {
	const char* dname = luaL_checkstring(L,1);
	const EVP_MD* md = EVP_get_digestbyname(dname);
	if (!md) {
		luaL_error(L,"not found digest algo %s",dname);
	}
	HashRef* ref = 
		new (lua_newuserdata(L,sizeof(HashRef))) 
			HashRef(new Hash());
	EVP_DigestInit(ref->get()->m_ctx,md);
	luaL_setmetatable(L,mt_name);
	return 1;
}

void Hash::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"update",&Hash::update);
	luabind::bind(L,"final",&Hash::final);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&HashRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}