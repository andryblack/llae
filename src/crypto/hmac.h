#ifndef __LLAE_CRYPTO_HMAC_H_INCLUDED__
#define __LLAE_CRYPTO_HMAC_H_INCLUDED__

#include "llae-private/mbedtls/md.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "llae/write_buffers.h"
#include "meta/object.h"
#include "lua/state.h"
#include "llae/promise.h"
#include "llae/sequental.h"


namespace crypto {

	/**
	* HMAC (Hash-based Message Authentication Code) functionality for message authentication.
	*/
	/// @luabind
	class hmac : public meta::object {
		META_OBJECT
	private:
		const mbedtls_md_info_t* m_info;
		mbedtls_md_context_t m_ctx;
		llae::sequental m_seq;
		hmac(const mbedtls_md_info_t* info);
		llae::result<void> start_impl(const llae::buffer_view& key);
		llae::result<void> update_impl(const llae::write_buffers& buffers);
		llae::result<llae::buffer_base_ptr> finish_impl();
	public:
		~hmac();

		/// Resets HMAC for reuse with a new key.
		/// @luabind
		llae::result<void> reset();
		llae::result<void> sync_start(const llae::buffer_base_ptr& key);
		/// Initializes HMAC with the provided key.
		/// @luabind(name=start,async=true)
		llae::result_promise_ptr<void> async_start(llae::loop& a,llae::buffer_base_ptr key);

		llae::result<void> sync_update(const llae::write_buffers& buffers);
		/// Updates HMAC with additional data.
		/// @lparam(data,string|llae.buffer_base) The data to add to the HMAC
		/// @luabind(name=update,async=true)
		llae::result_promise_ptr<void> lasync_update(lua::state& l);

		llae::result<llae::buffer_base_ptr> sync_finish();
		/// Finalizes HMAC and returns the authentication code.
		/// @luabind(name=finish,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::loop& a);
		
		/// Creates a new HMAC instance with the specified hash algorithm.
		/// @lparam(algorithm,string) The hash algorithm to use (e.g., 'SHA256', 'MD5', 'SHA1')
		/// @lreturn(instance,crypto.hmac?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};

}

#endif /*__LLAE_CRYPTO_MD_H_INCLUDED__*/
