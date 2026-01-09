#include "random.h"
#include "lua/bind.h"
#include "crypto.h"
#include "llae/buffer.h"

META_OBJECT_INFO(crypto::random,meta::object)
META_OBJECT_INFO(crypto::entropy,meta::object)

namespace crypto {


	entropy::entropy() {
		mbedtls_entropy_init( &m_entropy );
	}

	entropy::~entropy() {
		mbedtls_entropy_free( &m_entropy );
	}

	lua::multiret entropy::update_manual(lua::state& l) {
		auto data = llae::buffer_view::get(l,2,true);
		auto res = mbedtls_entropy_update_manual(&m_entropy,static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return mbedtls_result(l,res);
	}

	lua::multiret entropy::lnew(lua::state& l) {
		lua::push(l,entropy_ptr(new entropy( )));
		return {1};
	}

	void entropy::lbind(lua::state& l) {
		lua::bind::function(l,"new",&entropy::lnew);
		lua::bind::function(l,"update_manual",&entropy::update_manual);
	}
	
	random::random()  {
		mbedtls_ctr_drbg_init( &m_ctr_drbg );
	}

	random::~random() {
		mbedtls_ctr_drbg_free( &m_ctr_drbg );
	}

	int random::seed(const entropy_ptr& e, const llae::buffer_view& pers) {
		auto ee = e ? e : entropy_ptr(new entropy());
		return mbedtls_ctr_drbg_seed( &m_ctr_drbg, mbedtls_entropy_func, ee->get(),
                               (const unsigned char *) pers.get_base(),
                               pers.get_len() );
	}
	lua::multiret random::lseed(lua::state& l) {
		auto e = lua::stack<entropy_ptr>::get(l,2);
		auto b = llae::buffer_view::get(l,3,false);
		int ret = seed(e,b);
		return mbedtls_result(l,ret);
	}

	int random::read_func(void *p_rng,
                            unsigned char *output, size_t output_len) {
		auto self = static_cast<random*>(p_rng);
		return mbedtls_ctr_drbg_random(&self->m_ctr_drbg,output,output_len);
	}

	lua::multiret random::update(lua::state& l) {
		auto data = llae::buffer_view::get(l,2,true);
		auto res = mbedtls_ctr_drbg_update(&m_ctr_drbg,static_cast<const unsigned char*>(data.get_base()),data.get_len());
		return mbedtls_result(l,res);
	}

	lua::multiret random::lnew(lua::state& l) {
		lua::push(l,random_ptr(new random( )));
		return {1};
	}

	void random::lbind(lua::state& l) {
		lua::bind::function(l,"new",&random::lnew);
		lua::bind::function(l,"update",&random::update);
		lua::bind::function(l,"seed",&random::lseed);
	}

}
