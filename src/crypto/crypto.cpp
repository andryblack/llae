#include "crypto.h"
#include "llae/promise.h"
#include "llae/work.h"
#include "md.h"
#include "hmac.h"
#include "bignum.h"
#include "ecp.h"
#include "cipher.h"
#include "meta/object.h"
#include "pk.h"
#include "rsa.h"
#include "random.h"
#include "lua/bind.h"
#include "llae-private/mbedtls/error.h"
#include "llae-private/mbedtls/cipher.h"
#include "llae-private/mbedtls/hkdf.h"
#include "llae-private/zlib.h"

#include "llae/buffer.h"
#include "llae/app.h"
#include "llae/async_bind.h"
#include <cstdint>
#include <format>

META_OBJECT_INFO(crypto::status_error, llae::error)

namespace crypto {

	void push_error(lua::state& l,const char* fmt, int error) {
		char buffer[128] = {0};
		mbedtls_strerror(error,buffer,sizeof(buffer));
		l.pushfstring(fmt,error,buffer);
	}

	const std::string status_error::category = "crypto";
	std::string status_error::to_string() const {
		char buffer[128] = {0};
		mbedtls_strerror(get_code(),buffer,sizeof(buffer));
		return std::format("[crypto]:{}",buffer);
	}

	static uint32_t sync_crc32(uint32_t start,const llae::buffer_base_ptr& data) {
		return static_cast<uint32_t>(crc32(start,static_cast<const Bytef *>(data->get_base()),
			static_cast<uInt>(data->get_len())));
	}

	static llae::result_promise_ptr<uint32_t> async_crc32(llae::app& a,uint32_t start,llae::buffer_base_ptr data) {
		if (!data) {
			return llae::make_result_promise_string_error<uint32_t>("need data");
		}
		using work_t = llae::function_work<uint32_t>;
		return work_t::start(a, [start,ldata=std::move(data)](){
			return llae::result<uint32_t>(sync_crc32(start,ldata));
		});
	}



	static llae::result<llae::buffer_base_ptr> sync_hkdf(const mbedtls_md_info_t* md_info, const llae::buffer_base_ptr& bsalt,const llae::buffer_base_ptr& binfo,const llae::buffer_base_ptr& bkey, size_t osize) {
		auto result = llae::buffer::alloc(osize);
		const unsigned char *salt = nullptr;
		size_t salt_len = 0;
		if (bsalt) {
			salt = reinterpret_cast<const unsigned char*>(bsalt->get_base());
			salt_len = bsalt->get_len();
		}
		const unsigned char *info = nullptr;
		size_t info_len = 0;
		if (binfo) {
			info = reinterpret_cast<const unsigned char*>(binfo->get_base());
			info_len = binfo->get_len();
		}
		const unsigned char *key = nullptr;
		size_t key_len = 0;
		if (bkey) {
			key = reinterpret_cast<const unsigned char*>(bkey->get_base());
			key_len = bkey->get_len();
		}	
		auto status = mbedtls_hkdf(md_info,
			salt,salt_len,
			key,key_len,
			info,info_len,
			reinterpret_cast<unsigned char*>(result->get_base()),result->get_len());

		if (status == 0) {
			llae::buffer_base_ptr r = std::move(result);
			return llae::result<llae::buffer_base_ptr>(std::move(r));
		}
		return status_error::create(status);
	}


	static llae::result_promise_ptr<llae::buffer_base_ptr> lua_lhkdf(lua::state& l) {
		auto md_info = md::get_info(l,1);
		if (!md_info) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr>("unknown md algorithm");
		}
		auto salt = llae::buffer_base::get(l,2);
		auto info = llae::buffer_base::get(l,3);
		auto key = llae::buffer_base::get(l,4);
		auto osize = l.checkinteger(5);
		using work_t = llae::function_work<llae::buffer_base_ptr,llae::default_function_work_hold<llae::buffer_base_ptr,6>>;
		return work_t::start(llae::app::get(l), [md_info,lsalt = std::move(salt),linfo=std::move(info),lkey=std::move(key),osize]{
			return sync_hkdf(md_info, lsalt, linfo, lkey, osize);
		});
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
	llae::async_function(l,"crc32",&crypto::async_crc32);
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
	llae::async_function(l,"hkdf",&crypto::lua_lhkdf);

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
