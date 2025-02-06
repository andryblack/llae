#pragma once

#include <mbedtls/ctr_drbg.h>
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {

	class random : public meta::object {
		META_OBJECT
	private:
		mbedtls_ctr_drbg_context m_ctr_drbg;
	public:
		random();
		~random();
        
        mbedtls_ctr_drbg_context* get() { return &m_ctr_drbg; }

		lua::multiret update(lua::state& l);
		int randomize();

		static int read_func(void *p_rng,
                            unsigned char *output, size_t output_len);

		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& s);
	};

	using random_ptr = common::intrusive_ptr<random>;

}
