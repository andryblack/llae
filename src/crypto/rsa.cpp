#include "rsa.h"
#include "crypto.h"

META_OBJECT_INFO(crypto::rsa_base,meta::object)

namespace crypto {

	rsa_base::~rsa_base() {
		
	}

	lua::multiret rsa_base::set_padding(lua::state& l) {
		auto padding = static_cast<int>(l.checkinteger(2));
		auto hash_id = static_cast<mbedtls_md_type_t>(l.optinteger(3,MBEDTLS_MD_NONE));
		auto res = mbedtls_rsa_set_padding(m_ctx,padding,hash_id);
		return mbedtls_result(l,res);
	}

}
