#pragma once

#include "llae-private/mbedtls/rsa.h"
#include "meta/object.h"
#include "lua/state.h"

namespace crypto {

	/// @luabind
	class rsa_base : public meta::object {
		META_OBJECT
	protected:
		mbedtls_rsa_context* m_ctx = nullptr;
		explicit rsa_base() {}
	public:
		~rsa_base();

		/// @luabind
		lua::multiret set_padding(lua::state& l);
	};

}
