#ifndef __LLAE_SSL_CTX_H_INCLUDED__
#define __LLAE_SSL_CTX_H_INCLUDED__

#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

namespace ssl {

	class ctx : public meta::object {
		META_OBJECT
	private:
		mbedtls_entropy_context m_entropy;
		mbedtls_ctr_drbg_context m_ctr_drbg;
		mbedtls_x509_crt m_cacert;
	public:
		explicit ctx(  );
		~ctx();
		static int lnew(lua_State* L);
		static void lbind(lua::state& l);
		lua::multiret init(lua::state& l);
		int configure(mbedtls_ssl_config* conf);
	};
	typedef common::intrusive_ptr<ctx> ctx_ptr;

}

#endif /*__LLAE_SSL_CTX_H_INCLUDED__*/