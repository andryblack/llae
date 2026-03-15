#pragma once

#include "llae-private/mbedtls/ctr_drbg.h"
#include "llae-private/mbedtls/entropy.h"
#include "llae/buffer.h"
#include "meta/object.h"
#include "lua/state.h"
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
		entropy();
		~entropy();

		mbedtls_entropy_context* get() { return &m_entropy; }

		int read(unsigned char* dst, size_t len);

		/// @luabind
		lua::multiret update_manual(lua::state& l);
		/// Creates a new entropy source instance.
		/// @lreturn(result,crypto.entropy)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
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
		explicit random(entropy_ptr&& e);
		~random();
        
        mbedtls_ctr_drbg_context* get() { return &m_ctr_drbg; }

		/// Updates the random number generator with additional entropy.
		/// @lparam(data,string|llae.buffer_base) Additional entropy data
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret update(lua::state& l);

		int seed(const llae::buffer_view& pers);
		/// Seeds the random number generator.
		/// @lparam(entropy,crypto.entropy?) Optional entropy source
		/// @lparam(pers,string?) Optional persistent string
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind(name=seed)
		lua::multiret lseed(lua::state& l);

		static int read_func(void *p_rng,
                            unsigned char *output, size_t output_len);
		int read(unsigned char *output, size_t output_len);

		/// Creates a new random number generator.
		/// @lreturn(result,crypto.random?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};

	using random_ptr = common::intrusive_ptr<random>;

}
