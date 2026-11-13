#include "random.h"
#include "crypto.h"
#include "llae/buffer.h"
#include "llae-private/mbedtls/entropy.h"

META_OBJECT_INFO(crypto::random,meta::object)
META_OBJECT_INFO(crypto::entropy,meta::object)

namespace crypto {


	entropy::entropy() {
		mbedtls_entropy_init( &m_entropy );
	}

	entropy::~entropy() {
		mbedtls_entropy_free( &m_entropy );
	}

	llae::result<void> entropy::update_manual(const llae::buffer_view& data) {
		auto res = mbedtls_entropy_update_manual(&m_entropy,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return make_result(res);
	}


	int entropy::read(unsigned char* dst, size_t len) {
		return mbedtls_entropy_func(&m_entropy, dst,len);
	}

	random::random(crypto::entropy_ptr&& e) : m_enthropy(std::move(e))  {
		mbedtls_ctr_drbg_init( &m_ctr_drbg );
		if (!m_enthropy) {
			m_enthropy.reset( new entropy() );
		}
	}

	random::~random() {
		mbedtls_ctr_drbg_free( &m_ctr_drbg );
	}

	int random::entropy_func(void * data, unsigned char * dst, size_t len) {
		auto self = static_cast<random*>(data);
		return self->m_enthropy->read(dst,len);
	}

	llae::result<void> random::seed(const llae::buffer_view& pers) {
		auto status = mbedtls_ctr_drbg_seed( &m_ctr_drbg, &entropy_func, this,
							   (const unsigned char *) pers.get_base(),
							   pers.get_len() );
		return make_result(status);
	}

	int random::read_func(void *p_rng,
							unsigned char *output, size_t output_len) {
		auto self = static_cast<random*>(p_rng);
		return self->read(output,output_len);
	}

	int random::read(unsigned char *output, size_t output_len) {
		return mbedtls_ctr_drbg_random(&m_ctr_drbg,output,output_len);
	}

	llae::result<void> random::update(const llae::buffer_view& data) {
		auto res = mbedtls_ctr_drbg_update(&m_ctr_drbg,
			static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return make_result(res);
	}

}
