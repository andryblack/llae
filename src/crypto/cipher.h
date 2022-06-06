#pragma once

#include <mbedtls/cipher.h>

#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {
	class loop;
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
}

namespace crypto {

	class cipher : public meta::object {
		META_OBJECT
	private:
		mbedtls_cipher_context_t m_ctx;
		explicit cipher(const mbedtls_cipher_info_t* info);
		class async;
		class finish_async;
		class update_async;
		void on_completed(lua::state& l,int uvstatus,int mbedlsstatus,uv::buffer_ptr&& digest);
		lua::ref m_cont;
		//bool m_started = false;
        void release() { m_cont.release(); }
	public:
		~cipher();
		lua::multiret set_iv(lua::state& l);
		lua::multiret set_key(lua::state& l);
		lua::multiret set_padding(lua::state& l);
		lua::multiret reset(lua::state& l);
		lua::multiret update(lua::state& l);
		lua::multiret finish(lua::state& l);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};
	using cipher_ptr = common::intrusive_ptr<cipher>;

}