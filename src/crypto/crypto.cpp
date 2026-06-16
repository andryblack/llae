#include "crypto.h"
#include "llae/promise.h"
#include "llae/work.h"
#include "md.h"
#include "llae-private/mbedtls/error.h"
#include "llae-private/mbedtls/hkdf.h"
#include "llae-private/zlib.h"
#include "llae/buffer.h"
#include "llae/loop.h"
#include <cstdint>

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
		return std::string("[crypto]:") + buffer;
	}

	static uint32_t sync_crc32_impl(uint32_t start,const llae::buffer_base_ptr& data) {
		return static_cast<uint32_t>(crc32(start,static_cast<const Bytef *>(data->get_base()),
			static_cast<uInt>(data->get_len())));
	}

	llae::result_promise_ptr<uint32_t> async_crc32(llae::loop& a,uint32_t start,llae::buffer_base_ptr data) {
		if (!data) {
			return llae::make_result_promise_string_error<uint32_t>("need data");
		}
		using work_t = llae::function_work<uint32_t>;
		return work_t::start(a, [start,ldata=std::move(data)](){
			return llae::result<uint32_t>(sync_crc32_impl(start,ldata));
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


	llae::result_promise_ptr<llae::buffer_base_ptr> lua_lhkdf(lua::state& l) {
		auto md_info = md::get_info(l,1);
		if (!md_info) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr>("unknown md algorithm");
		}
		auto salt = llae::buffer_base::get(l,2);
		auto info = llae::buffer_base::get(l,3);
		auto key = llae::buffer_base::get(l,4);
		auto osize = l.checkinteger(5);
		constexpr size_t hold_size = sizeof(void*) == 4 ? 8 : 6;
		using work_t = llae::function_work<llae::buffer_base_ptr,llae::default_function_work_hold<llae::buffer_base_ptr,hold_size>>;
		return work_t::start(llae::loop::get(l), [md_info,lsalt = std::move(salt),linfo=std::move(info),lkey=std::move(key),osize]{
			return sync_hkdf(md_info, lsalt, linfo, lkey, osize);
		});
	}
}

