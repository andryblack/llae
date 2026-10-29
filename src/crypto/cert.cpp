#include "cert.h"
#include "crypto.h"
#include "llae/buffer.h"
#include "llae-private/mbedtls/md.h"
#include "llae-private/mbedtls/oid.h"
#include <string_view>

META_INFO(crypto::cert_time,void)
META_OBJECT_INFO(crypto::cert,meta::object)

namespace crypto {

	static llae::error_ptr not_loaded_error() {
		return status_error::create(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
	}

	static llae::result<std::string> dn_gets(const mbedtls_x509_name* dn) {
		std::string buffer(256,'\0');
		for (;;) {
			int ret = mbedtls_x509_dn_gets(buffer.data(),buffer.size(),dn);
			if (ret >= 0) {
				buffer.resize(static_cast<size_t>(ret));
				return buffer;
			}
			if (ret != MBEDTLS_ERR_X509_BUFFER_TOO_SMALL || buffer.size() >= 65536) {
				return status_error::create(ret);
			}
			buffer.resize(buffer.size() * 2);
		}
	}

	static llae::buffer_base_ptr copy_buf(const mbedtls_x509_buf& buf) {
		if (!buf.p || buf.len == 0) {
			return {};
		}
		return llae::buffer::hold(buf.p,buf.len);
	}

	static llae::result<uint32_t> name_hash_old(const mbedtls_x509_buf& name) {
		const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
		if (!info) {
			return status_error::create(MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE);
		}
		unsigned char md[16] = {0};
		int ret = mbedtls_md(info,name.p,name.len,md);
		if (ret != 0) {
			return status_error::create(ret);
		}
		uint32_t hash = static_cast<uint32_t>(md[0])
			| (static_cast<uint32_t>(md[1]) << 8)
			| (static_cast<uint32_t>(md[2]) << 16)
			| (static_cast<uint32_t>(md[3]) << 24);
		return hash;
	}

	cert::cert() {
		mbedtls_x509_crt_init(&m_crt);
	}

	cert::~cert() {
		mbedtls_x509_crt_free(&m_crt);
	}

	void cert::reset() {
		mbedtls_x509_crt_free(&m_crt);
		mbedtls_x509_crt_init(&m_crt);
		m_loaded = false;
	}

	llae::result<> cert::parse(const llae::buffer_view& data) {
		reset();

		if (data.empty()) {
			return status_error::create(MBEDTLS_ERR_X509_BAD_INPUT_DATA);
		}

		const unsigned char* buffer = static_cast<const unsigned char*>(data.get_base());
		size_t length = data.get_len();
		std::string pem;

		// mbedtls_x509_crt_parse requires a NUL-terminated buffer for PEM.
		if (buffer[0] != 0x30) {
			std::string_view view(static_cast<const char*>(data.get_base()),data.get_len());
			if (view.find("-----BEGIN CERTIFICATE-----") != std::string_view::npos) {
				pem.assign(view.data(),view.size());
				pem.push_back('\0');
				buffer = reinterpret_cast<const unsigned char*>(pem.data());
				length = pem.size();
			}
		}

		int ret = mbedtls_x509_crt_parse(&m_crt,buffer,length);
		if (ret != 0) {
			reset();
		} else {
			m_loaded = true;
		}
		return make_result(ret);
	}

	bool cert::is_loaded() const {
		return m_loaded;
	}

	int cert::get_version() const {
		return m_loaded ? m_crt.version : 0;
	}

	llae::result<std::string> cert::get_subject() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return dn_gets(&m_crt.subject);
	}

	llae::result<std::string> cert::get_issuer() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return dn_gets(&m_crt.issuer);
	}

	llae::result<llae::buffer_base_ptr> cert::get_serial() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return copy_buf(m_crt.serial);
	}

	cert_time cert::get_not_before() const {
		return m_loaded ? m_crt.valid_from : cert_time{};
	}

	cert_time cert::get_not_after() const {
		return m_loaded ? m_crt.valid_to : cert_time{};
	}

	std::string cert::get_signature_algorithm() const {
		if (!m_loaded) {
			return {};
		}
		const char* desc = nullptr;
		if (mbedtls_oid_get_sig_alg_desc(&m_crt.sig_oid,&desc) != 0 || !desc) {
			return {};
		}
		return desc;
	}

	llae::result<llae::buffer_base_ptr> cert::get_raw() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return copy_buf(m_crt.raw);
	}

	llae::result<llae::buffer_base_ptr> cert::get_subject_der() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return copy_buf(m_crt.subject_raw);
	}

	llae::result<llae::buffer_base_ptr> cert::get_issuer_der() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return copy_buf(m_crt.issuer_raw);
	}

	llae::result<llae::buffer_base_ptr> cert::get_public_key_der() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return copy_buf(m_crt.pk_raw);
	}

	llae::result<pk_ptr> cert::get_public_key() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		pk_ptr key(new pk());
		auto res = key->parse_public_key(llae::buffer_view(m_crt.pk_raw.p,m_crt.pk_raw.len));
		if (res.get_error()) {
			return res.get_error();
		}
		return key;
	}

	llae::result<uint32_t> cert::get_subject_hash_old() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return name_hash_old(m_crt.subject_raw);
	}

	llae::result<uint32_t> cert::get_issuer_hash_old() const {
		if (!m_loaded) {
			return not_loaded_error();
		}
		return name_hash_old(m_crt.issuer_raw);
	}

	bool cert::is_expired() const {
		return m_loaded && mbedtls_x509_time_is_past(&m_crt.valid_to);
	}

	bool cert::is_not_yet_valid() const {
		return m_loaded && mbedtls_x509_time_is_future(&m_crt.valid_from);
	}

	bool cert::is_valid_now() const {
		return m_loaded && !is_expired() && !is_not_yet_valid();
	}

}
