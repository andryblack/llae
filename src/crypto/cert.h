#pragma once

#include "crypto/pk.h"
#include "llae-private/mbedtls/x509_crt.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include <cstdint>
#include <string>

namespace crypto {

	class cert;
	using cert_ptr = common::intrusive_ptr<cert>;

#ifdef LUABIND_PARSE
	/// @luabind(all=true)
	struct cert_time {
		/// @luabind(type=integer)
		LUABIND_FIELD(year)
		/// @luabind(type=integer)
		LUABIND_FIELD(mon)
		/// @luabind(type=integer)
		LUABIND_FIELD(day)
		/// @luabind(type=integer)
		LUABIND_FIELD(hour)
		/// @luabind(type=integer)
		LUABIND_FIELD(min)
		/// @luabind(type=integer)
		LUABIND_FIELD(sec)
	};
#else
	using cert_time = mbedtls_x509_time;
#endif

	/**
	* X.509 certificate wrapper.
	* Certificates are loaded only from memory (DER or PEM), never from a file path.
	*/
	/// @luabind
	class cert : public meta::object {
		META_OBJECT
	private:
		mbedtls_x509_crt m_crt;
		bool m_loaded = false;

		void reset();
	public:
		/// Creates an empty certificate.
		/// Use parse() to load certificate data.
		/// @lreturn(instance,crypto.cert)
		/// @luabind(name=new)
		cert();

		~cert();

		/// Loads a certificate or certificate chain from DER or PEM data.
		/// Replaces any previously loaded certificate.
		/// @lparam(data,string|llae.buffer_base) The certificate data
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<> parse(const llae::buffer_view& data);

		/// Returns true if a certificate has been loaded.
		/// @luabind
		bool is_loaded() const;

		/// Returns the X.509 version (1, 2 or 3).
		/// @luabind
		int get_version() const;

		/// Returns the subject DN in OpenSSL-like printable form.
		/// @lreturn(subject,string?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<std::string> get_subject() const;

		/// Returns the issuer DN in OpenSSL-like printable form.
		/// @lreturn(issuer,string?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<std::string> get_issuer() const;

		/// Returns the raw certificate serial number.
		/// @lreturn(serial,llae.buffer_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<llae::buffer_base_ptr> get_serial() const;

		/// Returns the "not before" time.
		/// @lreturn(time,crypto.cert_time)
		/// @luabind
		cert_time get_not_before() const;

		/// Returns the "not after" time.
		/// @lreturn(time,crypto.cert_time)
		/// @luabind
		cert_time get_not_after() const;

		/// Returns the signature algorithm description.
		/// @luabind
		std::string get_signature_algorithm() const;

		/// Returns the whole certificate in DER form.
		/// @lreturn(der,llae.buffer_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<llae::buffer_base_ptr> get_raw() const;

		/// Returns the DER-encoded subject Name.
		/// @lreturn(der,llae.buffer_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<llae::buffer_base_ptr> get_subject_der() const;

		/// Returns the DER-encoded issuer Name.
		/// @lreturn(der,llae.buffer_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<llae::buffer_base_ptr> get_issuer_der() const;

		/// Returns the DER-encoded SubjectPublicKeyInfo.
		/// @lreturn(der,llae.buffer_base?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<llae::buffer_base_ptr> get_public_key_der() const;

		/// Returns the certificate public key as a crypto.pk object.
		/// @lreturn(key,crypto.pk?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<pk_ptr> get_public_key() const;

		/// Returns the old-style (MD5) subject hash used by OpenSSL's
		/// `x509 -subject_hash_old` and legacy c_rehash symlinks.
		/// @lreturn(hash,uint32_t?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<uint32_t> get_subject_hash_old() const;

		/// Returns the old-style (MD5) issuer hash.
		/// @lreturn(hash,uint32_t?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<uint32_t> get_issuer_hash_old() const;

		/// Returns true if the certificate is already expired.
		/// @luabind
		bool is_expired() const;

		/// Returns true if the certificate is not valid yet.
		/// @luabind
		bool is_not_yet_valid() const;

		/// Returns true if the certificate is valid at the current time.
		/// @luabind
		bool is_valid_now() const;

		const mbedtls_x509_crt& get() const { return m_crt; }
		mbedtls_x509_crt& get() { return m_crt; }
	};

}
