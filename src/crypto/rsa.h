#pragma once

#include "llae-private/mbedtls/rsa.h"
#include "meta/object.h"
#include "lua/state.h"

namespace crypto {

	/**
	* RSA-specific operations for public key cryptography.
	*/
	/// @luabind
	class rsa_base : public meta::object {
		META_OBJECT
	protected:
		mbedtls_rsa_context* m_ctx = nullptr;
		explicit rsa_base() {}
	public:
		~rsa_base();

		/// Sets the padding mode for RSA operations.
		/// @lparam(padding,integer) Padding mode (RSA_PKCS_V15 or RSA_PKCS_V21)
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret set_padding(lua::state& l);
	};

}
