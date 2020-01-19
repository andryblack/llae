#ifndef __CRYPTO_DECIPHER_H_INCLUDED__
#define __CRYPTO_DECIPHER_H_INCLUDED__

#include <openssl/evp.h>
#include "../ref_counter.h"

class Decipher : public RefCounter {
private:
	EVP_CIPHER_CTX* m_ctx;
public:
	Decipher();
	~Decipher();
public:
	static void lbind(lua_State* L);
	static int lnew(lua_State* L);
	static int update(lua_State* L);
	static int final(lua_State* L);
};
typedef Ref<Decipher> DecipherRef; 

#endif /*__CRYPTO_DECIPHER_H_INCLUDED__*/