#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/ecp.h"
#include "llae/buffer.h"
#include "lua/types.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"


namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
}

namespace crypto {

    class bignum;
	/// @luabind
    using bignum_ptr = common::intrusive_ptr<bignum>;

    class ecp_point;
	/// @luabind
	using ecp_point_ptr = common::intrusive_ptr<ecp_point>;

	/**
	* Elliptic Curve Point operations for cryptographic computations.
	*/
	/// @luabind
	class ecp_point : public meta::object {
		META_OBJECT
	private:
		mbedtls_ecp_point m_point;
	public:
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
		/// @luabind
		bool cmp(const ecp_point_ptr& pnt) const;
		/// Reads a point from string data.
		/// @lparam(data,string|llae.buffer_base) The input data containing the point
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret read_string(lua::state& l);
       
		/// Creates a new elliptic curve point instance.
		/// @lreturn(result,crypto.ecp_point)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
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

		/// Reads a point from binary data.
		/// @lparam(data,string|llae.buffer_base) The binary data containing the point
		/// @lreturn(result,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret point_read_binary(lua::state& l);
		/// Writes a point to binary format.
		/// @lparam(point,crypto.ecp_point) The elliptic curve point to write
		/// @lparam(format,integer?) Point format (ECP_PF_COMPRESSED or ECP_PF_UNCOMPRESSED)
		/// @lreturn(result,llae.buffer?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret point_write_binary(lua::state& l);
		/// Checks if a public key point is valid.
		/// @luabind
		bool check_pubkey(const ecp_point_ptr& pnt) const;
		/// Checks if a private key is valid.
		/// @luabind
        bool check_privkey(const bignum_ptr& pnt) const;
		/// Verifies an ECDSA signature.
		/// @lparam(hash,string|llae.buffer_base) The hash of the data that was signed
		/// @lparam(signature,string|llae.buffer_base) The signature to verify
		/// @lparam(r,crypto.bignum) The public key r component
		/// @lparam(s,crypto.bignum) The public key s component
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret ecdsa_verify(lua::state& l);
		/// Signs data using ECDSA.
		/// @lparam(hash,string|llae.buffer_base) The hash of the data to sign
		/// @lparam(privkey,crypto.bignum) The private key to sign with
		/// @lreturn(result,llae.buffer?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret ecdsa_sign(lua::state& l);
		/// Generates a new key pair.
		/// @lparam(random_data,string|llae.buffer_base?) Additional random data for key generation
		/// @lreturn(privkey,crypto.bignum?)
		/// @lreturn(pubkey,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret gen_keypair(lua::state& l);
		/// Generates a new private key.
		/// @lparam(random_data,string|llae.buffer_base?) Additional random data for key generation
		/// @lreturn(privkey,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret gen_privkey(lua::state& l);
		/// Generates a public key from a private key.
		/// @lparam(privkey,crypto.bignum) The private key
		/// @lreturn(result,crypto.ecp_point?)
		/// @lreturn(error,string?)
		/// @luabind
        lua::multiret gen_pubkey(lua::state& l);
		/// Sets additional random data for cryptographic operations.
		/// @luabind
        void set_random_data(const llae::buffer_view& data);
		/// @luabind
		void set_random(const random_ptr& r) { m_random = r; }
		/// Generates an ECDH keypair on an elliptic curve.
		/// @lreturn(privkey,crypto.bignum?)
		/// @lreturn(pubkey_or_error,crypto.ecp_point|string)
		/// @luabind
		lua::multiret ecdh_gen_public(lua::state& l);
		/// Computes the shared secret.
		/// @lparam(Q,crypto.ecp_point) public key
		/// @lparam(d,crypto.bignum) Our secret exponent (private key)
		/// @lreturn(secret,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind
		lua::multiret ecdh_compute_shared(lua::state& l);
		/// @luabind
        lua::multiret scalar_mul(lua::state& l);

		/// Creates a new elliptic curve instance with the specified group.
		/// @lparam(group_id,string) The elliptic curve group identifier
		/// @lreturn(result,crypto.ecp?)
		/// @lreturn(error,string?)
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};
	using ecp_ptr = common::intrusive_ptr<ecp>;
}
