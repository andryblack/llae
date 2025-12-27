#pragma once

#include "llae-private/mbedtls/rsa.h"
#include "meta/object.h"
#include "lua/state.h"

namespace crypto {

	class rsa_base : public meta::object {
		META_OBJECT
	protected:
		mbedtls_rsa_context* m_ctx = nullptr;
		explicit rsa_base() {}
	public:
		~rsa_base();

		lua::multiret set_padding(lua::state& l);

		static void lbind(lua::state& l);
	};

}
