#ifndef __LLAE_SSL_CTX_H_INCLUDED__
#define __LLAE_SSL_CTX_H_INCLUDED__

#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"
#include "crypto/random.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "llae-private/mbedtls/ssl.h"


namespace ssl {

	/// SSL context for managing SSL/TLS configuration and certificates.
	/// @luabind
	class ctx : public meta::object {
		META_OBJECT
	private:
	    crypto::random_ptr m_random;
		mbedtls_x509_crt m_cacert;
		static int rng_read(void* data,unsigned char* dst,size_t len);
	public:
		/// Default certificate authority file path for the current platform.
		/// @luabind
		static const char* default_cafile;

		/// Creates a new SSL context with optional random number generator.
		/// @lparam(random,crypto.random?) Optional random number generator
		/// @lreturn(instance,ssl.ctx)
		/// @luabind(name=new)
		explicit ctx( crypto::random_ptr&& random );
		~ctx();
		/// Initializes the SSL context with random number generation.
		/// @lparam(pers,string?) Optional persistent string
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<void> init(const llae::buffer_view& pers);
		/// Loads a certificate from buffer data (PEM or DER format).
		/// @lparam(cert_data,llae.buffer_base|string) The certificate data
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<void> load_cert(const llae::buffer_view& data);
		/// Loads system certificates from the operating system certificate store.
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret load_system_certs(lua::state& l);
		int configure(mbedtls_ssl_config* conf);
		/// Sets the debug threshold for SSL debugging output.
		/// @lparam(threshold,integer) Debug threshold level
		/// @luabind
		static void set_debug_threshold(int v);
	};
	typedef common::intrusive_ptr<ctx> ctx_ptr;

}

#endif /*__LLAE_SSL_CTX_H_INCLUDED__*/
