#pragma once

#include "llae-private/mbedtls/ctr_drbg.h"
#include "llae-private/mbedtls/entropy.h"
#include "llae/buffer.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {

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
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};
	using entropy_ptr = common::intrusive_ptr<entropy>;

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

		/// @luabind
		lua::multiret update(lua::state& l);

		int seed(const llae::buffer_view& pers);
		/// @luabind(name=seed)
		lua::multiret lseed(lua::state& l);

		static int read_func(void *p_rng,
                            unsigned char *output, size_t output_len);
		int read(unsigned char *output, size_t output_len);

		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};

	using random_ptr = common::intrusive_ptr<random>;

}
