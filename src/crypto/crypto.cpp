#include "crypto.h"
#include "md.h"
#include "hmac.h"
#include "bignum.h"
#include "ecp.h"
#include "cipher.h"
#include "pk.h"
#include "rsa.h"
#include "random.h"
#include "lua/bind.h"
#include "llae-private/mbedtls/error.h"
#include "llae-private/mbedtls/cipher.h"
#include "llae-private/mbedtls/hkdf.h"
#include "llae-private/zlib.h"

#include "uv/work.h"
#include "llae/buffer.h"
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
		llae::buffer_base_ptr m_data;
		lua::ref m_cont;
	public:
		explicit crc32_async(uint32_t start,llae::buffer_base_ptr&& data,lua::ref&& cont) : m_value(start),m_data(std::move(data)),m_cont(std::move(cont)) {}
		virtual void on_work() {
			m_value = static_cast<uint32_t>(crc32(m_value,static_cast<const Bytef *>(m_data->get_base()),
                            static_cast<uInt>(m_data->get_len())));
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
			uint32_t start = static_cast<uint32_t>(l.checkinteger(1));
			auto data = llae::buffer_base::get(l,2);

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


	class mbedtls_async : public uv::lua_cont_work {
	protected:
		int m_status = 0;
		virtual int push_result(lua::state& toth) {
			toth.pushboolean(true);
			return 1;
		}
	public:
		explicit mbedtls_async() {}
		virtual int resume_args(lua::state& toth,int uvstatus) override {
			int nres = 0;
			if (uvstatus<0) {
				toth.pushnil();
				uv::push_error(toth,uvstatus);
				return 2;
			} else if (m_status!=0) {
				toth.pushnil();
				push_error(toth,"update failed, code:%d, %s",m_status);
				return 2;
			} else {
				return push_result(toth);
			}
		}
	};

	class hkdf_async : public mbedtls_async {
	protected:
		const mbedtls_md_info_t* m_md_info;
		llae::buffer_base_ptr m_salt;
		llae::buffer_base_ptr m_info;
		llae::buffer_base_ptr m_key;
		llae::buffer_ptr m_result;
	public:
		explicit hkdf_async(const mbedtls_md_info_t* md_info, llae::buffer_base_ptr&& salt,llae::buffer_base_ptr&& info,llae::buffer_base_ptr&& key, size_t osize) : 
			m_md_info(md_info),
			m_salt(std::move(salt)),
			m_info(std::move(info)),
			m_key(std::move(key)) {
			m_result = llae::buffer::alloc(osize);
		}
		virtual void on_work() override {
			const unsigned char *salt = nullptr;
			size_t salt_len = 0;
			if (m_salt) {
				salt = reinterpret_cast<const unsigned char*>(m_salt->get_base());
				salt_len = m_salt->get_len();
			}
			const unsigned char *info = nullptr;
			size_t info_len = 0;
			if (m_info) {
				info = reinterpret_cast<const unsigned char*>(m_info->get_base());
				info_len = m_info->get_len();
			}
			const unsigned char *key = nullptr;
			size_t key_len = 0;
			if (m_key) {
				key = reinterpret_cast<const unsigned char*>(m_key->get_base());
				key_len = m_key->get_len();
			}	
			m_status = mbedtls_hkdf(m_md_info,
				salt,salt_len,
				key,key_len,
				info,info_len,
				reinterpret_cast<unsigned char*>(m_result->get_base()),m_result->get_len());
		}
		virtual int push_result(lua::state& toth) override {
			lua::push(toth,std::move(m_result));
			return 1;
		}
	};

	static lua::multiret lua_lhkdf(lua::state& l) {
		if (!l.isyieldable()) {
			l.pushnil();
			l.pushstring("hkdf is async");
			return {2};
		}
		{
			auto md_info = md::get_info(l,1);
			if (!md_info) {
				l.pushnil();
				l.pushstring("unknown md algorithm");
				return {2};
			}
			auto salt = llae::buffer_base::get(l,2);
			auto info = llae::buffer_base::get(l,3);
			auto key = llae::buffer_base::get(l,4);
			auto osize = l.checkinteger(5);
			common::intrusive_ptr<hkdf_async> req{new hkdf_async(md_info,std::move(salt),std::move(info),std::move(key),osize)};
			int r = req->queue_work_thread(l);
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
	lua::bind::object<crypto::cipher>::register_metatable(l,&crypto::cipher::lbind);
    lua::bind::object<crypto::bignum>::register_metatable(l,&crypto::bignum::lbind);
    lua::bind::object<crypto::ecp>::register_metatable(l,&crypto::ecp::lbind);
    lua::bind::object<crypto::ecp_point>::register_metatable(l,&crypto::ecp_point::lbind);
    lua::bind::object<crypto::pk>::register_metatable(l,&crypto::pk::lbind);
    lua::bind::object<crypto::rsa_base>::register_metatable(l,&crypto::rsa_base::lbind);
    lua::bind::object<crypto::entropy>::register_metatable(l,&crypto::entropy::lbind);
    lua::bind::object<crypto::random>::register_metatable(l,&crypto::random::lbind);
	l.createtable();
	lua::bind::function(l,"crc32",&crypto::lcrc32);
	lua::bind::object<crypto::md>::get_metatable(l);
	l.setfield(-2,"md");
	lua::bind::object<crypto::hmac>::get_metatable(l);
	l.setfield(-2,"hmac");
	lua::bind::object<crypto::cipher>::get_metatable(l);
	l.setfield(-2,"cipher");
    lua::bind::object<crypto::bignum>::get_metatable(l);
    l.setfield(-2,"bignum");
    lua::bind::object<crypto::ecp>::get_metatable(l);
    l.setfield(-2,"ecp");
    lua::bind::object<crypto::ecp_point>::get_metatable(l);
    l.setfield(-2,"ecp_point");
    lua::bind::object<crypto::pk>::get_metatable(l);
    l.setfield(-2,"pk");
    lua::bind::object<crypto::entropy>::get_metatable(l);
    l.setfield(-2,"entropy");
    lua::bind::object<crypto::random>::get_metatable(l);
    l.setfield(-2,"random");
	lua::bind::function(l,"hkdf",&crypto::lua_lhkdf);

#define BIND_M(M) l.pushinteger(MBEDTLS_ ## M);l.setfield(-2,#M);
    BIND_M(ECP_PF_COMPRESSED)
    BIND_M(ECP_PF_UNCOMPRESSED)
    BIND_M(DECRYPT)
    BIND_M(ENCRYPT)
    BIND_M(PADDING_PKCS7)
    BIND_M(PADDING_ONE_AND_ZEROS)
    BIND_M(PADDING_ZEROS_AND_LEN)
    BIND_M(PADDING_ZEROS)
    BIND_M(PADDING_NONE)
    BIND_M(RSA_PKCS_V15)
    BIND_M(RSA_PKCS_V21)
    BIND_M(MD_NONE) 	 /**< None. */
    BIND_M(MD_MD5)       /**< The MD5 message digest. */
    BIND_M(MD_SHA1)      /**< The SHA-1 message digest. */
    BIND_M(MD_SHA224)    /**< The SHA-224 message digest. */
    BIND_M(MD_SHA256)    /**< The SHA-256 message digest. */
    BIND_M(MD_SHA384)    /**< The SHA-384 message digest. */
    BIND_M(MD_SHA512)    /**< The SHA-512 message digest. */
    BIND_M(MD_RIPEMD160) /**< The RIPEMD-160 message digest. */
   return 1;
}
