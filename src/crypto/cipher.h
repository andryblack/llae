#pragma once

#include "llae-private/mbedtls/cipher.h"

#include "llae/buffer.h"
#include "llae/result.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/promise.h"
#include "llae/sequental.h"

namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
	class buffer_base;
	using buffer_base_ptr = common::intrusive_ptr<buffer_base>;
	class app;
	class write_buffers;
}

namespace crypto {

	/**
	* Symmetric encryption/decryption functionality using various cipher algorithms.
	* Supports algorithms like 'AES-256-CBC', 'AES-128-CBC', etc.
	*/
	/// @luabind
	class cipher : public meta::object {
		META_OBJECT
	private:
		mbedtls_cipher_context_t m_ctx;
		llae::sequental	m_seq;
		explicit cipher(const mbedtls_cipher_info_t* info);
		llae::result<llae::buffer_base_ptr> update_impl(const llae::write_buffers& buffers);
		llae::result<void> update_ad_impl(const llae::buffer_base_ptr& data);
		llae::result<llae::buffer_base_ptr> finish_impl();
		llae::result<llae::buffer_base_ptr> crypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& data);
		llae::result<llae::buffer_base_ptr> auth_encrypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
		llae::result<llae::buffer_base_ptr> auth_decrypt_impl(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
	public:
		~cipher();
		/// Sets the initialization vector (IV) for the cipher.
		/// @luabind
		llae::result<void> set_iv(const llae::buffer_view& iv);
		/// Sets the encryption/decryption key for the cipher.
		/// @luabind
		llae::result<void> set_key(const llae::buffer_view& key,mbedtls_operation_t op);
		/// Sets the padding mode for the cipher.
		/// @luabind
		llae::result<void> set_padding(mbedtls_cipher_padding_t padding);
		/// Resets the cipher for reuse.
		/// @luabind
		llae::result<void> reset(lua::state& l);
		llae::result<llae::buffer_base_ptr> sync_update(const llae::write_buffers& buffers);
		/// Processes data through the cipher (encrypt/decrypt).
		/// @lparam(data,string|llae.buffer_base) The data to process
		/// @luabind(name=update,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> lasync_update(lua::state& l);
		llae::result<void> sync_update_ad(const llae::buffer_base_ptr& data);
		/// Processes additional data through the cipher (encrypt/decrypt).
		/// @luabind(name=update_ad,async=true)
		llae::result_promise_ptr<void> async_update_ad(llae::app& a,llae::buffer_base_ptr&& data);
		/// Writes a tag from the cipher.
		/// @luabind
		llae::result<llae::buffer_base_ptr> write_tag(size_t tag_len);
		/// Checks a tag for the cipher.
		/// @luabind
		llae::result<void> check_tag(const llae::buffer_view& data);
		llae::result<llae::buffer_base_ptr> sync_finish();
		/// Finalizes the cipher operation and returns any remaining data.
		/// @luabind(name=finish,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::app& a);
		llae::result<llae::buffer_base_ptr> sync_crypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& data);
		/// Processes data through the cipher (encrypt/decrypt).
		/// @luabind(name=crypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_crypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr data);
		llae::result<llae::buffer_base_ptr> sync_auth_encrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
		/// Authenticates and encrypts data through the cipher.
		/// @luabind(name=auth_encrypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_auth_encrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len);
		llae::result<llae::buffer_base_ptr> sync_auth_decrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
		/// Authenticates and decrypts data through the cipher.
		/// @luabind(name=auth_decrypt,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_auth_decrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len);
		/// @luabind
		int get_block_size() const;
		/// @luabind
		int get_iv_size() const;
		
		/// Creates a new cipher instance with the specified algorithm.
		/// @lparam(algorithm,string) The cipher algorithm to use (e.g., 'AES-256-CBC')
		/// @lreturn(instance,crypto.cipher?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};
	using cipher_ptr = common::intrusive_ptr<cipher>;

}
