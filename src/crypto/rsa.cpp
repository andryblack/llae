#include "rsa.h"
#include "crypto.h"
#include "llae/buffer.h"
#include "llae/error.h"
#include "llae/promise.h"
#include "llae/work.h"


META_OBJECT_INFO(crypto::rsa_base,meta::object)

namespace crypto {

	using rsa_base_ptr = common::intrusive_ptr<rsa_base>;

	rsa_base::~rsa_base() {
		
	}

	llae::result<> rsa_base::set_padding(int padding,std::optional<mbedtls_md_type_t> hash_id) {
		if (!m_ctx) {
			return llae::string_error::create("RSA context not initialized");
		}
		auto res = mbedtls_rsa_set_padding(m_ctx,padding,hash_id.has_value() ? hash_id.value() : MBEDTLS_MD_NONE);
		return make_result(res);
	}

	llae::result<size_t> rsa_base::get_len() const {
		if (!m_ctx) {
			return llae::string_error::create("RSA context not initialized");
		}
		return mbedtls_rsa_get_len(m_ctx);
	}

	llae::result<llae::buffer_base_ptr> rsa_base::sync_public(const llae::buffer_view& buffer) {
		if (!m_ctx) {
			return llae::string_error::create("RSA context not initialized");
		}
		auto len = mbedtls_rsa_get_len(m_ctx);
		if (buffer.get_len() != len) {
			return llae::string_error::create("invalid buffer size");
		}
		llae::buffer_ptr result = llae::buffer::alloc(len);
		auto res = mbedtls_rsa_public(m_ctx,static_cast<const unsigned char*>(buffer.get_base()),static_cast<unsigned char*>(result->get_base()));
		if (res != 0) {
			return status_error::create(res);
		}
		return llae::buffer_base_ptr(result);
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> rsa_base::async_public(llae::loop& a,llae::buffer_base_ptr buffer) {
        if (!buffer) {
            return llae::result_promise_forward_error<llae::buffer_base_ptr>(llae::string_error::create("need buffer"));
        }
		using work_t = llae::function_work<llae::buffer_base_ptr>;
		return work_t::start(a, [self = rsa_base_ptr(this),lbuffer = std::move(buffer)](){
			return self->sync_public(*lbuffer);
		});
	}

}
