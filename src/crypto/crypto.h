#ifndef __LLAE_CRYPTO_CRYPTO_H_INCLUDED__
#define __LLAE_CRYPTO_CRYPTO_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "llae/error.h"
#include "llae/result.h"
#include "llae/promise.h"
#include "llae/buffer.h"
#include "llae-private/mbedtls/ecp.h"
#include "llae-private/mbedtls/cipher.h"
#include "llae-private/mbedtls/rsa.h"
#include "llae-private/mbedtls/md.h"

namespace llae {
	class loop;
	class buffer_base;
	using buffer_base_ptr = common::intrusive_ptr<buffer_base>;
}

namespace crypto {
	void push_error(lua::state& l,const char* fmt, int error);

	static inline lua::multiret mbedtls_result(lua::state& l,int res) {
		if (res == 0) {
			l.pushboolean(true);
			return {1};
		}
		l.pushnil();
		push_error(l,"failed code:%d, %s",res);
		return {2};
	}

	class status_error : public llae::code_error {
		META_OBJECT
    public:
    	static const std::string category;
    	explicit status_error(int status) : llae::code_error(status) {}
		virtual const std::string& get_category() const override { return category; }
    	virtual std::string to_string() const override;
		static llae::error_ptr create(int status) {
			return common::make_intrusive<status_error>(status);
		}
    };

	static inline llae::result<void> make_result(int status) {
		if (status != 0) {
			return status_error::create(status);
		}
		return llae::result<void>{};
	}

	/// Calculates CRC32 checksum of the input data.
	/// @luabind(name=crc32,async=true)
	llae::result_promise_ptr<uint32_t> async_crc32(llae::loop& a, uint32_t start, llae::buffer_base_ptr data);

	/// HKDF (HMAC-based Key Derivation Function) for key derivation.
	/// @lparam(md,string|integer) The message digest algorithm to use
	/// @lparam(salt,llae.buffer_base|string?) The salt value
	/// @lparam(info,llae.buffer_base|string?) The info value
	/// @lparam(key,llae.buffer_base|string?) The key value
	/// @lparam(osize,integer) The output size
	/// @luabind(name=hkdf,async=true)
	llae::result_promise_ptr<llae::buffer_base_ptr> lua_lhkdf(lua::state& l);

	/// @luabind
	constexpr int ECP_PF_COMPRESSED      = MBEDTLS_ECP_PF_COMPRESSED;
	/// @luabind
	constexpr int ECP_PF_UNCOMPRESSED    = MBEDTLS_ECP_PF_UNCOMPRESSED;
	/// @luabind
	constexpr int DECRYPT                = MBEDTLS_DECRYPT;
	/// @luabind
	constexpr int ENCRYPT                = MBEDTLS_ENCRYPT;
	/// @luabind
	constexpr int PADDING_PKCS7          = MBEDTLS_PADDING_PKCS7;
	/// @luabind
	constexpr int PADDING_ONE_AND_ZEROS  = MBEDTLS_PADDING_ONE_AND_ZEROS;
	/// @luabind
	constexpr int PADDING_ZEROS_AND_LEN  = MBEDTLS_PADDING_ZEROS_AND_LEN;
	/// @luabind
	constexpr int PADDING_ZEROS          = MBEDTLS_PADDING_ZEROS;
	/// @luabind
	constexpr int PADDING_NONE           = MBEDTLS_PADDING_NONE;
	/// @luabind
	constexpr int RSA_PKCS_V15           = MBEDTLS_RSA_PKCS_V15;
	/// @luabind
	constexpr int RSA_PKCS_V21           = MBEDTLS_RSA_PKCS_V21;
	/// @luabind
	constexpr int MD_NONE                = MBEDTLS_MD_NONE;
	/// @luabind
	constexpr int MD_MD5                 = MBEDTLS_MD_MD5;
	/// @luabind
	constexpr int MD_SHA1                = MBEDTLS_MD_SHA1;
	/// @luabind
	constexpr int MD_SHA224              = MBEDTLS_MD_SHA224;
	/// @luabind
	constexpr int MD_SHA256              = MBEDTLS_MD_SHA256;
	/// @luabind
	constexpr int MD_SHA384              = MBEDTLS_MD_SHA384;
	/// @luabind
	constexpr int MD_SHA512              = MBEDTLS_MD_SHA512;
	/// @luabind
	constexpr int MD_RIPEMD160           = MBEDTLS_MD_RIPEMD160;
}

#endif /*__LLAE_CRYPTO_CRYPTO_H_INCLUDED__*/
