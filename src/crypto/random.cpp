#include "random.h"
#include "lua/bind.h"
#include "crypto.h"
#include "uv/buffer.h"

META_OBJECT_INFO(crypto::random,meta::object)

namespace crypto {

	
	random::random() {
		mbedtls_ctr_drbg_init( &m_ctr_drbg );
	}

	random::~random() {
		mbedtls_ctr_drbg_free( &m_ctr_drbg );
	}

	
	int random::randomize() {
		unsigned char buf[128];
		for (size_t i=0;i<sizeof(buf);++i) {
			buf[i] = ::rand();
		}
		return mbedtls_ctr_drbg_update(&m_ctr_drbg,buf,sizeof(buf));
	}

	int random::read_func(void *p_rng,
                            unsigned char *output, size_t output_len) {
		auto self = static_cast<random*>(p_rng);
		return mbedtls_ctr_drbg_random(&self->m_ctr_drbg,output,output_len);
	}

	lua::multiret random::update(lua::state& l) {
		auto data = uv::buffer_view::get(l,2,true);
		auto res = mbedtls_ctr_drbg_update(&m_ctr_drbg,static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return mbedtls_result(l,res);
	}

	lua::multiret random::lnew(lua::state& l) {
		lua::push(l,random_ptr(new random()));
		return {1};
	}

	void random::lbind(lua::state& l) {
		lua::bind::function(l,"new",&random::lnew);
		lua::bind::function(l,"update",&random::update);
	}

}
