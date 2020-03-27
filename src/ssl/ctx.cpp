#include "ctx.h"
#include "lua/stack.h"
#include "lua/bind.h"

META_OBJECT_INFO(ssl::ctx,meta::object)

namespace ssl {

	static const char *pers = "ssl_client_llae";

	ctx::ctx() {
		mbedtls_entropy_init( &m_entropy );
		mbedtls_ctr_drbg_init( &m_ctr_drbg );
	}

	ctx::~ctx() {
		mbedtls_ctr_drbg_free( &m_ctr_drbg );
		mbedtls_entropy_free( &m_entropy );
	}

	int ctx::configure(mbedtls_ssl_config* conf) {
		mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, &m_ctr_drbg );
		return 0;
   	}

	lua::multiret ctx::init(lua::state& l) {
		int ret = mbedtls_ctr_drbg_seed( &m_ctr_drbg, mbedtls_entropy_func, &m_entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) );
		if (ret != 0) {
			l.pushnil();
			l.pushfstring("mbedtls_ctr_drbg_seed failed, code:%d",ret);
			return {2};
		}
		l.pushboolean(true);
		return {1};
	}


	int ctx::lnew(lua_State* L) {
		lua::state l(L);
		common::intrusive_ptr<ctx> c{new ctx()};
		lua::push(l,std::move(c));
		return 1;
	}

	void ctx::lbind(lua::state& l) {
		lua::bind::function(l,"new",&ctx::lnew);
		lua::bind::function(l,"init",&ctx::init);
	}

}