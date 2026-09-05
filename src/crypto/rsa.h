#pragma once

#include "llae-private/mbedtls/rsa.h"
#include "meta/object.h"
#include "llae/result.h"
#include "llae/buffer.h"
#include "llae/promise.h"
#include <optional>


namespace llae {
	class loop;
}

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

		/// @luabind
		llae::result<size_t> get_len() const;

		/// Sets the padding mode for RSA operations.
		/// @lparam(padding,integer) Padding mode (RSA_PKCS_V15 or RSA_PKCS_V21)
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<> set_padding(int padding,std::optional<mbedtls_md_type_t> hash_id);

		/// Performs an RSA public key operation.
		/// @luabind
		llae::result<llae::buffer_base_ptr> sync_public(const llae::buffer_view& buffer);
		/// Performs an RSA public key operation.
		/// @luabind(name=public,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_public(llae::loop& a,llae::buffer_base_ptr buffer);
	};

}
