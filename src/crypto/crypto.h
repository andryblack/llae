#ifndef __LLAE_CRYPTO_CRYPTO_H_INCLUDED__
#define __LLAE_CRYPTO_CRYPTO_H_INCLUDED__

#include "lua/state.h"

namespace crypto {
	void push_error(lua::state& l,const char* fmt, int error);

}

#endif /*__LLAE_CRYPTO_CRYPTO_H_INCLUDED__*/