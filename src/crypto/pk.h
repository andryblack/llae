#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/pk.h"
#include "llae/result.h"
#include "meta/object.h"
#include "llae/buffer.h"
#include "llae/promise.h"
#include <optional>

namespace llae {
	class loop;
}

namespace crypto {
	class pk;
	using pk_ptr = common::intrusive_ptr<pk>;
	class rsa_base;
	using rsa_base_ptr = common::intrusive_ptr<rsa_base>;
	/**
	* Public Key operations for RSA and other public key cryptography.
	*/
	/// @luabind
	class pk : public meta::object {
		META_OBJECT
	private:
		mbedtls_pk_context m_ctx;
	public:
		/// @luabind(name=new)
		pk();
		~pk();

		/// Parses a public key from key data.
		/// @lparam(key_data,string|llae.buffer_base) The public key data
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<> parse_public_key(const llae::buffer_view& data);
		/// Parses a private key from key data.
		/// @lparam(key_data,string|llae.buffer_base) The private key data
		/// @lparam(password,string?) Optional password for encrypted keys
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<> parse_private_key(const llae::buffer_view& data, std::optional<llae::buffer_view> password);
		/// Gets the name/type of the public key.
		/// @lreturn(name,string?)
		/// @lreturn(error,string?)
		/// @luabind
		std::string get_name();
		/// Gets the RSA context if the key is an RSA key.
		/// @lreturn(rsa,crypto.rsa_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<rsa_base_ptr> get_rsa();

		llae::result<llae::buffer_base_ptr> sync_encrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random);
		/// Encrypts data using the public key.
		/// @luabind(name=encrypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_encrypt(llae::loop& a,llae::buffer_base_ptr buffer,random_ptr random);

		llae::result<llae::buffer_base_ptr> sync_decrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random);
		/// Decrypts data using the private key.
		/// @luabind(name=decrypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_decrypt(llae::loop& a,llae::buffer_base_ptr buffer,random_ptr random);
		
	};

}
