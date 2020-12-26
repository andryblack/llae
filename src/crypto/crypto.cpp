#include "crypto.h"
#include "md.h"
#include "lua/bind.h"
#include <mbedtls/error.h>

namespace crypto {

	void push_error(lua::state& l,const char* fmt, int error) {
		char buffer[128] = {0};
		mbedtls_strerror(error,buffer,sizeof(buffer));
		l.pushfstring(fmt,error,buffer);
	}

}

int luaopen_crypto(lua_State* L) {
	lua::state l(L);

	lua::bind::object<crypto::md>::register_metatable(l,&crypto::md::lbind);
	l.createtable();
	lua::bind::object<crypto::md>::get_metatable(l);
	l.setfield(-2,"md");

	return 1;
}