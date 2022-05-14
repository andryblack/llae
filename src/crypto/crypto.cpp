#include "crypto.h"
#include "md.h"
#include "hmac.h"
#include "bignum.h"
#include "ecp.h"
#include "lua/bind.h"
#include <mbedtls/error.h>
#include <zlib.h>

#include "uv/work.h"
#include "uv/buffer.h"
#include "uv/luv.h"
#include "llae/app.h"

namespace crypto {

	void push_error(lua::state& l,const char* fmt, int error) {
		char buffer[128] = {0};
		mbedtls_strerror(error,buffer,sizeof(buffer));
		l.pushfstring(fmt,error,buffer);
	}

	class crc32_async : public uv::work {
	protected:
		uint32_t m_value;
		uv::buffer_ptr m_data;
		lua::ref m_cont;
	public:
		explicit crc32_async(uint32_t start,uv::buffer_ptr&& data,lua::ref&& cont) : m_value(start),m_data(std::move(data)),m_cont(std::move(cont)) {}
		virtual void on_work() {
			m_value = crc32(m_value,static_cast<const Bytef *>(m_data->get_base()),m_data->get_len());
		}
		virtual void on_after_work(int status) {
            if (llae::app::closed(get_loop())) {
                m_data.reset();
                m_cont.release();
            } else {
                uv::loop loop(get_loop());
                llae::app::get(loop).resume(m_cont,m_value);
			}
		}
		void reset(lua::state& l) {
			m_cont.reset(l);
		}
	};

	static lua::multiret lcrc32(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("crc32 is async");
			return {2};
		}
		{
			uint32_t start = l.checkinteger(1);
			auto data = uv::buffer::get(l,2);

			l.pushthread();
			lua::ref cont;
			cont.set(l);

			common::intrusive_ptr<crc32_async> req{new crc32_async(start,std::move(data),std::move(cont))};

			int r = req->queue_work(llae::app::get(l).loop());
			if (r < 0) {
				req->reset(l);
				l.pushnil();
				uv::push_error(l,r);
				return {2};
			} 
		}
		l.yield(0);
		return {0};
	}

}

int luaopen_crypto(lua_State* L) {
	lua::state l(L);

	lua::bind::object<crypto::md>::register_metatable(l,&crypto::md::lbind);
	lua::bind::object<crypto::hmac>::register_metatable(l,&crypto::hmac::lbind);
    lua::bind::object<crypto::bignum>::register_metatable(l,&crypto::bignum::lbind);
    lua::bind::object<crypto::ecp>::register_metatable(l,&crypto::ecp::lbind);
    lua::bind::object<crypto::ecp_point>::register_metatable(l,&crypto::ecp_point::lbind);
	l.createtable();
	lua::bind::function(l,"crc32",&crypto::lcrc32);
	lua::bind::object<crypto::md>::get_metatable(l);
	l.setfield(-2,"md");
	lua::bind::object<crypto::hmac>::get_metatable(l);
	l.setfield(-2,"hmac");
    lua::bind::object<crypto::bignum>::get_metatable(l);
    l.setfield(-2,"bignum");
    lua::bind::object<crypto::ecp>::get_metatable(l);
    l.setfield(-2,"ecp");
    lua::bind::object<crypto::ecp_point>::get_metatable(l);
    l.setfield(-2,"ecp_point");

#define BIND_M(M) l.pushinteger(MBEDTLS_ ## M);l.setfield(-2,#M);
    BIND_M(ECP_PF_COMPRESSED)
    BIND_M(ECP_PF_UNCOMPRESSED)
    
	return 1;
}
