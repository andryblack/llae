#pragma once

#include "llae-private/mbedtls/ctr_drbg.h"
#include "llae-private/mbedtls/entropy.h"
#include "llae/buffer.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {

	class entropy : public meta::object {
		META_OBJECT
	private:
		mbedtls_entropy_context m_entropy;
	public:
		entropy();
		~entropy();

		mbedtls_entropy_context* get() { return &m_entropy; }

		lua::multiret update_manual(lua::state& l);
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& s);
	};
	using entropy_ptr = common::intrusive_ptr<entropy>;

	class random : public meta::object {
		META_OBJECT
	private:
		mbedtls_ctr_drbg_context m_ctr_drbg;
	public:
		explicit random();
		~random();
        
        mbedtls_ctr_drbg_context* get() { return &m_ctr_drbg; }

		lua::multiret update(lua::state& l);

		int seed(const entropy_ptr& e, const llae::buffer_view& pers);
		lua::multiret lseed(lua::state& l);

		static int read_func(void *p_rng,
                            unsigned char *output, size_t output_len);
		int read(unsigned char *output, size_t output_len);

		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& s);
	};

	using random_ptr = common::intrusive_ptr<random>;

}
