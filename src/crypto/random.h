#pragma once

#include "llae-private/mbedtls/ctr_drbg.h"
#include "llae-private/mbedtls/entropy.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"

namespace crypto {

	/**
	* Entropy source for cryptographically secure random number generation.
	*/
	/// @luabind
	class entropy : public meta::object {
		META_OBJECT
	private:
		mbedtls_entropy_context m_entropy;
	public:
		/// Creates a new entropy source instance.
		/// @luabind
		entropy();
		~entropy();

		mbedtls_entropy_context* get() { return &m_entropy; }

		int read(unsigned char* dst, size_t len);

		/// Adds additional entropy to the source.
		/// @lparam(data,string|llae.buffer_base) Additional entropy data
		/// @luabind
		llae::result<void> update_manual(const llae::buffer_view& data);
	};
	using entropy_ptr = common::intrusive_ptr<entropy>;

	/**
	* Cryptographically secure random number generation.
	*/
	/// @luabind
	class random : public meta::object {
		META_OBJECT
	private:
		mbedtls_ctr_drbg_context m_ctr_drbg;
		entropy_ptr m_enthropy;

		static int entropy_func(void *, unsigned char *, size_t);
	public:
		/// Creates a new random number generator.
		/// @lparam(entropy,crypto.entropy?) Optional entropy source
		/// @lreturn(instance,crypto.random)
		/// @luabind
		explicit random(crypto::entropy_ptr&& e);
		~random();

		mbedtls_ctr_drbg_context* get() { return &m_ctr_drbg; }

		/// Updates the random number generator with additional entropy.
		/// @lparam(data,string|llae.buffer_base) Additional entropy data
		/// @luabind
		llae::result<void> update(const llae::buffer_view& data);

		/// Seeds the random number generator.
		/// @lparam(pers,string|llae.buffer_base?) Optional personalization string
		/// @luabind(name=seed)
		llae::result<void> seed(const llae::buffer_view& pers);

		static int read_func(void *p_rng,
							unsigned char *output, size_t output_len);
		int read(unsigned char *output, size_t output_len);
	};

	using random_ptr = common::intrusive_ptr<random>;

}
