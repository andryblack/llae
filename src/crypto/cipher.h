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
		llae::result<void> set_iv(const llae::buffer_view& iv);
		llae::result<void> set_key(const llae::buffer_view& key,mbedtls_operation_t op);
		llae::result<void> set_padding(mbedtls_cipher_padding_t padding);
		llae::result<void> reset(lua::state& l);
		llae::result<llae::buffer_base_ptr> sync_update(const llae::write_buffers& buffers);
		llae::result_promise_ptr<llae::buffer_base_ptr> lasync_update(lua::state& l);
		llae::result<void> sync_update_ad(const llae::buffer_base_ptr& data);
		llae::result_promise_ptr<void> async_update_ad(llae::app& a,llae::buffer_base_ptr&& data);
		llae::result<llae::buffer_base_ptr> write_tag(size_t tag_len);
		llae::result<void> check_tag(const llae::buffer_view& data);
		llae::result<llae::buffer_base_ptr> sync_finish();
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::app& a);
		llae::result<llae::buffer_base_ptr> sync_crypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& data);
		llae::result_promise_ptr<llae::buffer_base_ptr> async_crypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr data);
		llae::result<llae::buffer_base_ptr> sync_auth_encrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
		llae::result_promise_ptr<llae::buffer_base_ptr> async_auth_encrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len);
		llae::result<llae::buffer_base_ptr> sync_auth_decrypt(const llae::buffer_base_ptr& iv, const llae::buffer_base_ptr& ad, const llae::buffer_base_ptr& buffer, size_t tag_len);
		llae::result_promise_ptr<llae::buffer_base_ptr> async_auth_decrypt(llae::app& a, llae::buffer_base_ptr iv, llae::buffer_base_ptr ad, llae::buffer_base_ptr buffer, size_t tag_len);
		int get_block_size() const;
		int get_iv_size() const;
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};
	using cipher_ptr = common::intrusive_ptr<cipher>;

}