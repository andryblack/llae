#include "ctx.h"
#include "crypto/random.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "crypto/crypto.h"
#include "llae/buffer.h"
#include "llae-private/mbedtls/debug.h"
#include "llae-private/mbedtls/error.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

META_OBJECT_INFO(ssl::ctx,meta::object)

namespace ssl {

	
#if defined(__linux__)
	static const char* default_cafile = "/etc/ssl/certs/ca-certificates.crt";
#elif defined(_WIN32)
	static const char* default_cafile = nullptr;
#else
	static const char* default_cafile = "/etc/ssl/cert.pem";
#endif
	using crypto::push_error;
	

	ctx::ctx( crypto::random_ptr&& r) : m_random(std::move(r)) {
        if (!m_random) {
            m_random.reset(new crypto::random( crypto::entropy_ptr{} ));
        }
		
		mbedtls_x509_crt_init( &m_cacert );
        //mbedtls_debug_set_threshold(3);
	}

	ctx::~ctx() {
		mbedtls_x509_crt_free( &m_cacert );
		
	}

	int ctx::rng_read(void* data,unsigned char* dst,size_t len) {
		return static_cast<ctx*>(data)->m_random->read(dst,len);
	}

	int ctx::configure(mbedtls_ssl_config* conf) {
        mbedtls_ssl_conf_rng( conf, &ctx::rng_read, this );
		mbedtls_ssl_conf_ca_chain( conf, &m_cacert, NULL );
		mbedtls_ssl_conf_authmode( conf, MBEDTLS_SSL_VERIFY_REQUIRED );
		
		return 0;
   	}

   	void ctx::set_debug_threshold(int v) {
   		std::cout << "set_debug_threshold: " << v << std::endl;
   		mbedtls_debug_set_threshold(v);
   	}

	lua::multiret ctx::init(lua::state& l) {
		return m_random->lseed(l);
	}

	lua::multiret ctx::load_cert(lua::state& l) {
		auto data = lua::stack<llae::buffer_ptr>::get(l,2);
		if (!data) {
			l.pushnil();
			l.pushstring("need buffer with cert");
			return {2};
		}

        if (data->find("-----BEGIN ")) {
        	data = data->realloc(data->get_len()+1);
        	static_cast<unsigned char*>(data->get_base())[data->get_len()]=0;
        	data->set_len(data->get_len()+1);
        }
		

		int ret;
		if( ( ret = mbedtls_x509_crt_parse( &m_cacert, static_cast<const unsigned char*>(data->get_base()), data->get_len() ) ) != 0 ) {
		    l.pushnil();
			push_error(l,"mbedtls_x509_crt_parse failed, code:%d, %s",ret);
			return {2};
		}

		l.pushboolean(true);
		return {1};
	}

#ifdef _WIN32
	struct cert_store {
		HCERTSTORE store = NULL;
		~cert_store() {
			close();
		}
		void close() {
			if (store) {
				CertCloseStore(store,0);
			}
			store = NULL;
		}
	};
	struct cert_context {
		PCCERT_CONTEXT context = NULL;
		bool need_free = false;
		~cert_context() {
			free();
		}
		void free() {
			if (need_free) {
				CertFreeCertificateContext(context);
				need_free = false;
			}
		}
		bool next(cert_store& store) {
			//free();
			context = CertEnumCertificatesInStore(store.store,context);
			need_free = !!context;
			return need_free;
		}
	};
#endif
	lua::multiret ctx::load_system_certs(lua::state& l) {
#ifdef _WIN32
		cert_store store;
		store.store = CertOpenSystemStore(0, "ROOT");
    	if (store.store == NULL) {
        	l.pushboolean(false);
        	l.pushstring("failed open root cert strore");
        	return {2};	
        }

        bool found = false;
        cert_context context;
        while (context.next(store)) {
        	// Convert the certificate to DER format (example)
        	// unsigned char *der_data = NULL;
        	// DWORD der_size = 0;
        	// if (!CertGetCertificateContextProperty(context.context, CERT_ அப்படியே_PROPERTY, NULL, &der_size)) {
            // 	// Handle error
            // 	continue;
        	// }
        	// der_data = (unsigned char*)malloc(der_size);
        	// if (!CertGetCertificateContextProperty(context.context, CERT_ அப்படியே_PROPERTY, der_data, &der_size)) {
            // 	//Handle error
            // 	free(der_data);
            // 	continue;
        	// }
        	if (context.context->dwCertEncodingType == X509_ASN_ENCODING) {
        		const auto* cert = reinterpret_cast<unsigned char*>(context.context->pbCertEncoded);
        		const auto certlen = context.context->cbCertEncoded;
        		//std::cout << "parse cert" << std::endl;
        		auto ret = mbedtls_x509_crt_parse( &m_cacert, cert, certlen );
        		if( ret != 0 ) {
		 		    //std::cout << "failed parse cert" << std::endl;
		 		    //l.pushnil();
					//push_error(l,"mbedtls_x509_crt_parse failed, code:%d, %s",ret);
					//return {2};
				} else {
					found = true;
				}
        	} else {
        		//std::cout << "skip cert by type" << std::endl;
        	}
        }
        context.free();
        store.close();
        if (found) {
			l.pushboolean(true);
			return {1};
		} else {
			l.pushboolean(false);
			l.pushstring("not found certificates");
			return {2};
		}
#else
		return {0};
#endif
	}

	void ctx::lbind(lua::state& l) {
		lua::bind::value(l,"default_cafile",default_cafile);
        lua::bind::constructor<ctx,crypto::random_ptr&&>(l);
		lua::bind::function(l,"init",&ctx::init);
		lua::bind::function(l,"set_debug_threshold",&ctx::set_debug_threshold);
		lua::bind::function(l,"load_cert",&ctx::load_cert);
		lua::bind::function(l,"load_system_certs",&ctx::load_system_certs);
	}

}
