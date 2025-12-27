#ifndef __LLAE_SSL_CTX_H_INCLUDED__
#define __LLAE_SSL_CTX_H_INCLUDED__

#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "crypto/random.h"
#include "llae-private/mbedtls/ssl.h"


namespace ssl {

	class ctx : public meta::object {
		META_OBJECT
	private:
	    crypto::random_ptr m_random;
		mbedtls_x509_crt m_cacert;
	public:
		explicit ctx( crypto::random_ptr&& random );
		~ctx();
		static void lbind(lua::state& l);
		lua::multiret init(lua::state& l);
		lua::multiret load_cert(lua::state& l);
		lua::multiret load_system_certs(lua::state& l);
		int configure(mbedtls_ssl_config* conf);
		static void set_debug_threshold(int v);
	};
	typedef common::intrusive_ptr<ctx> ctx_ptr;

}

#endif /*__LLAE_SSL_CTX_H_INCLUDED__*/
