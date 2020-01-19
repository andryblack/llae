#ifndef __CRYPTO_HASH_H_INCLUDED__
#define __CRYPTO_HASH_H_INCLUDED__

#include <openssl/evp.h>
#include "../ref_counter.h"

class Hash : public RefCounter {
private:
	EVP_MD_CTX* m_ctx;
public:
	Hash();
	~Hash();
public:
	static void lbind(lua_State* L);
	static int lnew(lua_State* L);
	static int update(lua_State* L);
	static int final(lua_State* L);
};
typedef Ref<Hash> HashRef; 

#endif /*__CRYPTO_HASH_H_INCLUDED__*/