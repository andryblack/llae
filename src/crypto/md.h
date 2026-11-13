#ifndef __LLAE_CRYPTO_MD_H_INCLUDED__
#define __LLAE_CRYPTO_MD_H_INCLUDED__

#include "llae-private/mbedtls/md.h"
#include "llae/buffer.h"
#include "llae/error.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/sequental.h"

#include <string_view>

namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
	class loop;
	class write_buffers;
}

namespace crypto {

	class md;
	using md_ptr = common::intrusive_ptr<md>;

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
		llae::result<void> update_impl(const llae::buffer_base_ptr& data);
		llae::result<void> update_impl(const llae::write_buffers& data);
		llae::result<llae::buffer_base_ptr> finish_impl();
	public:
		~md();

		/// Finds the mbedTLS message digest description by its name.
		static const mbedtls_md_info_t* find_info(std::string_view algorithm);
		/// Finds the mbedTLS message digest description by its type.
		static const mbedtls_md_info_t* find_info(mbedtls_md_type_t type);

		/// Creates a new message digest instance with the specified algorithm type.
		static md_ptr create(mbedtls_md_type_t type);
		/// Creates a new message digest instance with the specified algorithm.
		/// @lparam(arg,integer|string) The hash algorithm to use ('MD5', 'SHA256', ... or a crypto.MD_* value)
		/// @lreturn(result,crypto.md?)
		/// @lreturn(error,string?)
		/// @luabind(name=new,wrapper=crypto::lua_md_create)
		static md_ptr create(const char* type);

		/// Updates the digest with additional data.
		/// @lparam(data,string|llae.buffer_base|string[]) The data to add to the digest
		/// @luabind(name=update,async=true,wrapper=crypto::lua_md_update)
		llae::result_promise_ptr<void> async_update(llae::loop& a,llae::buffer_base_ptr&& data);

		llae::result<void> sync_update(const llae::buffer_base_ptr& data);
		llae::result<llae::buffer_base_ptr> sync_finish();

		/// Finalizes the digest and returns the hash result.
		/// @luabind(name=finish,async=true)
		llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::loop& a);

		/// Returns the digest size in bytes for the specified algorithm.
		/// @lparam(algorithm,integer|string) The hash algorithm to use ('MD5', 'SHA256', ... or a crypto.MD_* value)
		/// @lreturn(size,integer?)
		/// @lreturn(error,string?)
		/// @luabind(name=get_length,wrapper=crypto::lua_md_get_length)
		static llae::result<size_t> get_length(std::string_view algorithm);
		/// Returns the digest size in bytes for the specified algorithm type.
		static llae::result<size_t> get_length(mbedtls_md_type_t type);

		/// Async update for a bundle of Lua buffers (used by the Lua bindings).
		llae::result_promise_ptr<void> async_update_buffers(llae::loop& a,llae::write_buffers&& buffers);
	};

}

#endif /*__LLAE_CRYPTO_MD_H_INCLUDED__*/
