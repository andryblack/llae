#ifndef __CRYPTO_HMAC_H_INCLUDED__
#define __CRYPTO_HMAC_H_INCLUDED__

#include <openssl/hmac.h>
#include "../ref_counter.h"

class Hmac : public RefCounter {
private:
	HMAC_CTX* m_ctx;
public:
	Hmac();
	~Hmac();
public:
	static void lbind(lua_State* L);
	static int lnew(lua_State* L);
	static int update(lua_State* L);
	static int final(lua_State* L);
};
typedef Ref<Hmac> HmacRef; 

#endif /*__CRYPTO_HMAC_H_INCLUDED__*/