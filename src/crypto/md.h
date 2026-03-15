#ifndef __LLAE_CRYPTO_MD_H_INCLUDED__
#define __LLAE_CRYPTO_MD_H_INCLUDED__

#include "llae-private/mbedtls/md.h"
#include "llae/buffer.h"
#include "llae/error.h"
#include "llae/write_buffers.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/sequental.h"

namespace uv {
	class loop;
}
namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
}

namespace crypto {

	/**
	* Message digest functionality for hashing data using various algorithms.
	* Supported algorithms: 'NONE', 'MD5', 'SHA1', 'SHA224', 'SHA256', 'SHA384', 'SHA512', 'RIPEMD160'
	*/
	/// @luabind
	class md : public meta::object {
		META_OBJECT
	private:
		const mbedtls_md_info_t* m_info;
		mbedtls_md_context_t m_ctx;
		bool m_stated = false;
		llae::sequental m_seq;
		md(const mbedtls_md_info_t* info);

		friend common::intrusive_maker;
		llae::error_ptr try_start();
		llae::result<void> update_impl(const llae::write_buffers& data);
		llae::result<void> update_impl(const llae::buffer_base_ptr& data);
		llae::result<llae::buffer_base_ptr> finish_impl();
	public:
		~md();

		static common::intrusive_ptr<md> create(mbedtls_md_type_t type);
		static common::intrusive_ptr<md> create(const char* type);

		/// Updates the digest with additional data.
		/// @lparam(data,string|llae.buffer_base) The data to add to the digest
		/// @luabind(name=update,async=true)
		llae::result_promise_ptr<void> lasync_update(lua::state& l);
		
		llae::result<void> sync_update(const llae::buffer_base_ptr& data);
		llae::result<void> sync_update(const llae::write_buffers& data);
		llae::result<llae::buffer_base_ptr> sync_finish();

		llae::result_promise_ptr<void> async_update(llae::app& a,llae::buffer_base_ptr&& data);
		/// Finalizes the digest and returns the hash result.
		/// @luabind(name=finish,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::app& a);
		
		static const mbedtls_md_info_t* get_info(lua::state& l, int idx);
		/// @luabind
		static lua::multiret get_length(lua::state& l);
		/// Creates a new message digest instance with the specified algorithm.
		/// @lparam(algorithm,string) The hash algorithm to use (e.g., 'MD5', 'SHA256', 'SHA1')
		/// @lreturn(instance,crypto.md?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		};

}

#endif /*__LLAE_CRYPTO_MD_H_INCLUDED__*/
