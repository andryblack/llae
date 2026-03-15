#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/pk.h"
#include "llae/result.h"
#include "meta/object.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "llae/buffer.h"
#include "llae/promise.h"

namespace crypto {

	class pk : public meta::object {
		META_OBJECT
	private:
		mbedtls_pk_context m_ctx;
	public:
		pk();
		~pk();

		lua::multiret parse_public_key(lua::state& l);
		lua::multiret get_name(lua::state& l);
		lua::multiret get_rsa(lua::state& l);

		llae::result<llae::buffer_base_ptr> sync_encrypt(const llae::buffer_base_ptr& buffer,const random_ptr& random);
		llae::result_promise_ptr<llae::buffer_base_ptr> async_encrypt(llae::app& a,llae::buffer_base_ptr buffer,random_ptr random);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};

}
