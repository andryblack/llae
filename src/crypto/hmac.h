#ifndef __LLAE_CRYPTO_HMAC_H_INCLUDED__
#define __LLAE_CRYPTO_HMAC_H_INCLUDED__

#include "llae-private/mbedtls/md.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "meta/object.h"
#include "llae/promise.h"
#include "llae/sequental.h"
#include "common/intrusive_ptr.h"

#include <string_view>

namespace llae {
	class loop;
	class write_buffers;
}

namespace crypto {

	class hmac;
	using hmac_ptr = common::intrusive_ptr<hmac>;

	/**
	* HMAC (Hash-based Message Authentication Code) functionality for message authentication.
	*/
	/// @luabind
	class hmac : public meta::object {
		META_OBJECT
	private:
		friend common::intrusive_maker;
		const mbedtls_md_info_t* m_info;
		mbedtls_md_context_t m_ctx;
		llae::sequental m_seq;
		hmac(const mbedtls_md_info_t* info);
		llae::result<void> start_impl(const llae::buffer_view& key);
		llae::result<void> update_impl(const llae::write_buffers& buffers);
		llae::result<void> update_impl(const llae::buffer_base_ptr& data);
		llae::result<llae::buffer_base_ptr> finish_impl();
	public:
		~hmac();

		/// Creates a new HMAC instance with the specified hash algorithm type.
		static hmac_ptr create(mbedtls_md_type_t type);
		/// Creates a new HMAC instance with the specified hash algorithm.
		/// @lparam(arg,integer|string) The hash algorithm to use ('SHA256', 'MD5', ... or a crypto.MD_* value)
		/// @lreturn(result,crypto.hmac?)
		/// @lreturn(error,string?)
		/// @luabind(name=new,wrapper=crypto::lua_hmac_create)
		static hmac_ptr create(const char* type);

		/// Resets HMAC for reuse with a new key.
		/// @luabind
		llae::result<void> reset();
		llae::result<void> sync_start(const llae::buffer_base_ptr& key);
		/// Initializes HMAC with the provided key.
		/// @lparam(key,string|llae.buffer_base) The key to initialize HMAC with
		/// @luabind(name=start,async=true)
		llae::result_promise_ptr<void> async_start(llae::loop& a,llae::buffer_base_ptr key);

		/// Updates HMAC with additional data.
		/// @lparam(data,string|llae.buffer_base|string[]) The data to add to the HMAC
		/// @luabind(name=update,async=true,wrapper=crypto::lua_hmac_update)
		llae::result_promise_ptr<void> async_update(llae::loop& a,llae::buffer_base_ptr data);

		llae::result<llae::buffer_base_ptr> sync_finish();
		/// Finalizes HMAC and returns the authentication code.
		/// @luabind(name=finish,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::loop& a);

		/// Async update for a bundle of Lua buffers (used by the Lua bindings).
		llae::result_promise_ptr<void> async_update_buffers(llae::loop& a,llae::write_buffers&& buffers);
	};

}

#endif /*__LLAE_CRYPTO_HMAC_H_INCLUDED__*/
