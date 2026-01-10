#pragma once

#include "llae-private/mbedtls/cipher.h"

#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {
	class loop;
}

namespace llae {
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
		class buffers_async;
		class finish_async;
		class update_async;
		class update_ad_async;
		class crypt_async;
		class auth_encrypt_async;
		class auth_decrypt_async;
		void on_completed(lua::state& l,int uvstatus,int mbedlsstatus,llae::buffer_ptr&& digest);
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
		lua::multiret update_ad(lua::state& l);
		lua::multiret write_tag(lua::state& l);
		lua::multiret check_tag(lua::state& l);
		lua::multiret finish(lua::state& l);
		lua::multiret crypt(lua::state& l);
		lua::multiret auth_encrypt(lua::state& l);
		lua::multiret auth_decrypt(lua::state& l);
		int get_block_size() const;
		int get_iv_size() const;
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};
	using cipher_ptr = common::intrusive_ptr<cipher>;

}