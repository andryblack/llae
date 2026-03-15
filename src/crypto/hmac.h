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

		llae::result<void> reset();
		llae::result<void> sync_start(const llae::buffer_base_ptr& key);
		llae::result_promise_ptr<void> async_start(llae::app& a,llae::buffer_base_ptr key);

		llae::result<void> sync_update(const llae::write_buffers& buffers);
		llae::result_promise_ptr<void> lasync_update(lua::state& l);

		llae::result<llae::buffer_base_ptr> sync_finish();
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::app& a);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};

}

#endif /*__LLAE_CRYPTO_MD_H_INCLUDED__*/
