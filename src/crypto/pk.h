#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/pk.h"
#include "llae/result.h"
#include "meta/object.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/buffer.h"
#include "llae/promise.h"

namespace llae {
	class loop;
}

namespace crypto {

	/**
	* Public Key operations for RSA and other public key cryptography.
	*/
	/// @luabind
	class pk : public meta::object {
		META_OBJECT
	private:
		mbedtls_pk_context m_ctx;
	public:
		pk();
		~pk();

		/// Parses a public key from key data.
		/// @lparam(key_data,string|llae.buffer_base) The public key data
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret parse_public_key(lua::state& l);
		/// Gets the name/type of the public key.
		/// @lreturn(name,string?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret get_name(lua::state& l);
		/// Gets the RSA context if the key is an RSA key.
		/// @lreturn(rsa,crypto.rsa_base?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret get_rsa(lua::state& l);

		llae::result<llae::buffer_base_ptr> sync_encrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random);
		/// Encrypts data using the public key.
		/// @luabind(name=encrypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_encrypt(llae::loop& a,llae::buffer_base_ptr buffer,random_ptr random);
		
		/// Creates a new public key instance.
		/// @lreturn(result,crypto.pk)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};

}
