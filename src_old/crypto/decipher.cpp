#include "decipher.h"
#include "../luabind.h"
#include <new>

static const char* mt_name = "llae.crypto.Decipher";

Decipher::Decipher() : m_ctx(EVP_CIPHER_CTX_new()) {

}

Decipher::~Decipher() {
	EVP_CIPHER_CTX_free(m_ctx);
}

int Decipher::update(lua_State* L) {
	Decipher* self = DecipherRef::get_ptr(L,1);
	size_t ilen = 0;
	const unsigned char* idata = reinterpret_cast<const unsigned char*>(luaL_checklstring(L,2,&ilen));
	luaL_Buffer buf;
	int osize = ilen + 256;
    unsigned char* odata = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,osize));
	
	int len = EVP_DecryptUpdate(self->m_ctx,odata,&osize,idata,ilen);
	if (len < 0) {
		luaL_error(L,"Decipher::update error");
	}
	luaL_pushresultsize(&buf,len);
	return 1;
}

int Decipher::final(lua_State* L) {
	Decipher* self = DecipherRef::get_ptr(L,1);
	luaL_Buffer buf;
	int size = 256;
    unsigned char* data = reinterpret_cast<unsigned char*>(luaL_buffinitsize(L,&buf,size));
	int len = EVP_DecryptFinal(self->m_ctx,data,&size);
	if (len < 0) {
		luaL_error(L,"Decipher::final error");
	}
	luaL_pushresultsize(&buf,len);
	return 1;
}

int Decipher::lnew(lua_State* L) {
	const char* ciphername = luaL_checkstring(L,1);
	const EVP_CIPHER* cipher = EVP_get_cipherbyname(ciphername);
	if (!cipher) {
		luaL_error(L,"not found cipher algo %s",ciphername);
	}
	DecipherRef* ref = 
		new (lua_newuserdata(L,sizeof(DecipherRef))) 
			DecipherRef(new Decipher());
	const unsigned char* key = reinterpret_cast<const unsigned char*>(lua_tostring(L,2));
	const unsigned char* iv = reinterpret_cast<const unsigned char*>(lua_tostring(L,3));
	EVP_DecryptInit_ex(ref->get()->m_ctx,cipher,0,key,iv);
	luaL_setmetatable(L,mt_name);
	return 1;
}

void Decipher::lbind(lua_State* L) {
	luaL_newmetatable(L,mt_name);
	lua_newtable(L);
	luabind::bind(L,"update",&Decipher::update);
	luabind::bind(L,"final",&Decipher::final);
	lua_setfield(L,-2,"__index");
	lua_pushcfunction(L,&DecipherRef::gc);
	lua_setfield(L,-2,"__gc");
	lua_pop(L,1);
}