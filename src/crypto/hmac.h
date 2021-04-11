#ifndef __LLAE_CRYPTO_HMAC_H_INCLUDED__
#define __LLAE_CRYPTO_HMAC_H_INCLUDED__

#include <mbedtls/md.h>
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

	class hmac : public meta::object {
		META_OBJECT
	private:
		mbedtls_md_context_t m_ctx;
		hmac(const mbedtls_md_info_t* info);
		class async;
		class start_async;
		class finish_async;
		class update_async;
		void on_start_completed(lua::state& l,int uvstatus,int mbedlsstatus);
		void on_update_completed(lua::state& l,int uvstatus,int mbedlsstatus);
		void on_finish_completed(uv::loop& l,int uvstatus,int mbedlsstatus,uv::buffer_ptr&& digest);
		lua::ref m_cont;
		void release() { m_cont.release(); }
	public:
		~hmac();
		lua::multiret reset(lua::state& l);
		lua::multiret start(lua::state& l);
		lua::multiret update(lua::state& l);
		lua::multiret finish(lua::state& l);
		
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};

}

#endif /*__LLAE_CRYPTO_MD_H_INCLUDED__*/
