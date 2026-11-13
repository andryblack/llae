#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/ecp.h"
#include "llae/buffer.h"
#include "llae/result.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"

#include <optional>
#include <string_view>


namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
	class buffer_base;
	using buffer_base_ptr = common::intrusive_ptr<buffer_base>;
}

namespace crypto {

	class bignum;
	/// @luabind
	using bignum_ptr = common::intrusive_ptr<bignum>;

	class ecp_point;
	/// @luabind
	using ecp_point_ptr = common::intrusive_ptr<ecp_point>;

	class ecp;
	using ecp_ptr = common::intrusive_ptr<ecp>;

	/// Elliptic curve key pair.
	struct ecp_keypair {
		bignum_ptr private_key;
		ecp_point_ptr public_key;
	};

	/// ECDSA signature values.
	struct ecdsa_signature {
		bignum_ptr r;
		bignum_ptr s;
	};

	/**
	* Elliptic Curve Point operations for cryptographic computations.
	*/
	/// @luabind
	class ecp_point : public meta::object {
		META_OBJECT
	private:
		mbedtls_ecp_point m_point;
	public:
		/// Creates a new elliptic curve point instance.
		/// @lreturn(instance,crypto.ecp_point)
		/// @luabind
		ecp_point();
		~ecp_point();

		const mbedtls_ecp_point* get() const { return &m_point;}
		mbedtls_ecp_point* get() { return &m_point;}

		/// Sets the point to zero (point at infinity).
		/// @luabind
		void set_zero();
		/// Checks if the point is zero (point at infinity).
		/// @luabind
		bool is_zero();
		/// Compares this point with another point.
		/// @lparam(point,crypto.ecp_point) The point to compare with
		/// @luabind
		bool cmp(const ecp_point_ptr& pnt) const;
		/// Reads a point from string data.
		/// @lparam(x,string) The X coordinate
		/// @lparam(y,string) The Y coordinate
		/// @lparam(radix,integer?) The radix of the coordinates (10 by default)
		/// @luabind
		llae::result<void> read_string(std::string_view x, std::string_view y, std::optional<int> radix);
	};


	/**
	* Elliptic Curve operations for cryptographic computations including ECDSA signing and verification.
	*/
	/// @luabind
	class ecp : public meta::object {
		META_OBJECT
	private:
		mbedtls_ecp_group m_group;
		ecp();
		int load_group(mbedtls_ecp_group_id grp_id);
		static int rng_func(void *, unsigned char *, size_t);
		int rng_gen(unsigned char * buffer, size_t size);
		llae::buffer_ptr m_random_data;
		random_ptr m_random;
	public:
		~ecp();

		/// Creates a new elliptic curve instance with the specified group.
		/// @lparam(group_id,string) The elliptic curve group identifier
		/// @lreturn(result,crypto.ecp?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static llae::result<ecp_ptr> create(std::string_view group_id);

		/// Reads a point from binary data.
		/// @lparam(data,string|llae.buffer_base) The binary data containing the point
		/// @lreturn(result,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<ecp_point_ptr> point_read_binary(const llae::buffer_view& data);
		/// Writes a point to binary format.
		/// @lparam(point,crypto.ecp_point) The elliptic curve point to write
		/// @lparam(format,integer?) Point format (ECP_PF_COMPRESSED or ECP_PF_UNCOMPRESSED)
		/// @luabind
		llae::result<llae::buffer_base_ptr> point_write_binary(const ecp_point_ptr& point,
			std::optional<int> format);
		/// Checks if a public key point is valid.
		/// @lparam(point,crypto.ecp_point) The public key point
		/// @luabind
		bool check_pubkey(const ecp_point_ptr& pnt) const;
		/// Checks if a private key is valid.
		/// @lparam(privkey,crypto.bignum) The private key
		/// @luabind
		bool check_privkey(const bignum_ptr& pnt) const;
		/// Verifies an ECDSA signature.
		/// @lparam(hash,string|llae.buffer_base) The hash of the data that was signed
		/// @lparam(pubkey,crypto.ecp_point) The public key point
		/// @lparam(r,crypto.bignum) The signature r component
		/// @lparam(s,crypto.bignum) The signature s component
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<bool> ecdsa_verify(const llae::buffer_view& hash, const ecp_point_ptr& Q,
			const bignum_ptr& r, const bignum_ptr& s);
		/// Signs data using ECDSA.
		/// @lparam(hash,string|llae.buffer_base) The hash of the data to sign
		/// @lparam(privkey,crypto.bignum) The private key to sign with
		/// @lparam(hash_alg,string?) The deterministic hash algorithm name
		/// @lreturn(r,crypto.bignum?)
		/// @lreturn(s,crypto.bignum?)
		/// @luabind
		llae::result<ecdsa_signature> ecdsa_sign(const llae::buffer_view& hash, const bignum_ptr& d,
			std::optional<std::string_view> hash_alg);
		/// Generates a new key pair.
		/// @lreturn(privkey,crypto.bignum?)
		/// @lreturn(pubkey,crypto.ecp_point?)
		/// @luabind
		llae::result<ecp_keypair> gen_keypair();
		/// Generates a new private key.
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<bignum_ptr> gen_privkey();
		/// Generates a public key from a private key.
		/// @lparam(privkey,crypto.bignum) The private key
		/// @lreturn(result,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<ecp_point_ptr> gen_pubkey(const bignum_ptr& d);
		/// Sets additional random data for cryptographic operations.
		/// @lparam(data,string|llae.buffer_base) Additional random data
		/// @luabind
		void set_random_data(const llae::buffer_view& data);
		/// Sets the random number generator for cryptographic operations.
		/// @lparam(random,crypto.random) The random number generator
		/// @luabind
		void set_random(const random_ptr& r) { m_random = r; }
		/// Generates an ECDH keypair on an elliptic curve.
		/// @lreturn(privkey,crypto.bignum?)
		/// @lreturn(pubkey,crypto.ecp_point?)
		/// @luabind
		llae::result<ecp_keypair> ecdh_gen_public();
		/// Computes the shared secret.
		/// @lparam(Q,crypto.ecp_point) public key
		/// @lparam(d,crypto.bignum) Our secret exponent (private key)
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<bignum_ptr> ecdh_compute_shared(const ecp_point_ptr& Q, const bignum_ptr& d);
		/// Multiplies a point by a scalar.
		/// @lparam(point,crypto.ecp_point) The point to multiply
		/// @lparam(d,crypto.bignum) The scalar
		/// @lreturn(result,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<ecp_point_ptr> scalar_mul(const ecp_point_ptr& point, const bignum_ptr& d);
	};
}
