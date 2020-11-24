#include "ctx.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include <mbedtls/debug.h>
#include <mbedtls/error.h>
#include <iostream>

META_OBJECT_INFO(ssl::ctx,meta::object)

namespace ssl {

	static const char *pers = "ssl_client_llae";
#ifdef __linux__
	static const char* default_cafile = "/etc/ssl/certs/ca-certificates.crt";
#else
	static const char* default_cafile = "/etc/ssl/cert.pem";
#endif

	void push_error(lua::state& l,const char* fmt, int error) {
		char buffer[128] = {0};
		mbedtls_strerror(error,buffer,sizeof(buffer));
		l.pushfstring(fmt,error,buffer);
	}

	ctx::ctx() {
		mbedtls_entropy_init( &m_entropy );
		mbedtls_ctr_drbg_init( &m_ctr_drbg );
		mbedtls_x509_crt_init( &m_cacert );
        //mbedtls_debug_set_threshold(3);
	}

	ctx::~ctx() {
		mbedtls_x509_crt_free( &m_cacert );
		mbedtls_ctr_drbg_free( &m_ctr_drbg );
		mbedtls_entropy_free( &m_entropy );
	}

	int ctx::configure(mbedtls_ssl_config* conf) {
		mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, &m_ctr_drbg );
		mbedtls_ssl_conf_ca_chain( conf, &m_cacert, NULL );
		mbedtls_ssl_conf_authmode( conf, MBEDTLS_SSL_VERIFY_REQUIRED );
		
		return 0;
   	}

   	void ctx::set_debug_threshold(int v) {
   		std::cout << "set_debug_threshold: " << v << std::endl;
   		mbedtls_debug_set_threshold(v);
   	}

	lua::multiret ctx::init(lua::state& l) {
		const char* cafile = l.optstring(2,default_cafile);
		int ret = mbedtls_ctr_drbg_seed( &m_ctr_drbg, mbedtls_entropy_func, &m_entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) );
		if (ret != 0) {
			l.pushnil();
			push_error(l,"mbedtls_ctr_drbg_seed failed, code:%d, %s",ret);
			return {2};
		}

		if( ( ret = mbedtls_x509_crt_parse_file( &m_cacert, cafile ) ) != 0 ) {
		    l.pushnil();
			push_error(l,"mbedtls_x509_crt_parse_file failed, code:%d, %s",ret);
			return {2};
		}

		l.pushboolean(true);
		return {1};
	}


	int ctx::lnew(lua_State* L) {
		lua::state l(L);
		common::intrusive_ptr<ctx> c{new ctx()};
		lua::push(l,std::move(c));
		return 1;
	}

	void ctx::lbind(lua::state& l) {
		lua::bind::function(l,"new",&ctx::lnew);
		lua::bind::function(l,"init",&ctx::init);
		lua::bind::function(l,"set_debug_threshold",&ctx::set_debug_threshold);
	}

}
