#pragma once

#include <mbedtls/pk.h>
#include "meta/object.h"
#include "lua/state.h"
#include "lua/ref.h"
#include "uv/buffer.h"

namespace crypto {

	class pk : public meta::object {
		META_OBJECT
	private:
		mbedtls_pk_context m_ctx;
		class async;
		class decrypt_async;
		class encrypt_async;
		lua::ref m_cont;
		void on_completed(lua::state& l,int uvstatus,int mbedlsstatus,uv::buffer_base_ptr&& result);
		void release() { m_cont.release(); }
	public:
		pk();
		~pk();

		lua::multiret parse_public_key(lua::state& l);
		lua::multiret get_name(lua::state& l);
		lua::multiret get_rsa(lua::state& l);
		lua::multiret encrypt(lua::state& l);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};

}
