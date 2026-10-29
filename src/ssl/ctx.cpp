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
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

META_OBJECT_INFO(ssl::ctx,meta::object)

namespace ssl {

#if defined(__linux__)
	const char* ctx::default_cafile = "/etc/ssl/certs/ca-certificates.crt";
#elif defined(_WIN32)
	const char* ctx::default_cafile = nullptr;
#else
	const char* ctx::default_cafile = "/etc/ssl/cert.pem";
#endif

	static std::string ssl_error_string(const char* what,int code) {
		char buffer[128] = {0};
		mbedtls_strerror(code,buffer,sizeof(buffer));
		return std::string(what) + ", code:" + std::to_string(code) + ", " + buffer;
	}

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

	llae::result<void> ctx::init(const llae::buffer_view& pers) {
		return crypto::make_result(m_random->seed(pers));
	}

	llae::result<void> ctx::load_cert(const llae::buffer_view& data) {
		if (data.empty()) {
			return llae::string_error::create("need buffer with cert");
		}

		const void* base = data.get_base();
		size_t len = data.get_len();
		std::string pem;
		if (std::string_view(static_cast<const char*>(base),len).find("-----BEGIN ") != std::string_view::npos) {
			pem.assign(static_cast<const char*>(base),len);
			pem.push_back('\0');
			base = pem.data();
			len = pem.size();
		}

		int ret = mbedtls_x509_crt_parse(&m_cacert,static_cast<const unsigned char*>(base),len);
		if (ret != 0) {
			return llae::string_error::create(ssl_error_string("mbedtls_x509_crt_parse failed",ret));
		}
		return llae::result<void>{};
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

}
