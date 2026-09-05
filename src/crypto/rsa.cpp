#include "rsa.h"
#include "crypto.h"

META_OBJECT_INFO(crypto::rsa_base,meta::object)

namespace crypto {

	rsa_base::~rsa_base() {
		
	}

	llae::result<> rsa_base::set_padding(int padding,std::optional<mbedtls_md_type_t> hash_id) {
		auto res = mbedtls_rsa_set_padding(m_ctx,padding,hash_id.has_value() ? hash_id.value() : MBEDTLS_MD_NONE);
		return make_result(res);
	}

}
