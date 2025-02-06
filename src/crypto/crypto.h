#ifndef __LLAE_CRYPTO_CRYPTO_H_INCLUDED__
#define __LLAE_CRYPTO_CRYPTO_H_INCLUDED__

#include "lua/state.h"

namespace crypto {
	void push_error(lua::state& l,const char* fmt, int error);

	static inline lua::multiret mbedtls_result(lua::state& l,int res) {
		if (res == 0) {
			l.pushboolean(true);
			return {1};
		}
		l.pushnil();
		push_error(l,"failed code:%d, %s",res);
		return {2};
	}
}

#endif /*__LLAE_CRYPTO_CRYPTO_H_INCLUDED__*/
