#pragma once

#include "crypto/random.h"
#include "llae-private/mbedtls/ecp.h"
#include "lua/types.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "lua/state.h"
#include "lua/ref.h"

namespace uv {
	class loop;
}

namespace llae {
	class buffer;
	using buffer_ptr = common::intrusive_ptr<buffer>;
}

namespace crypto {

    class bignum;
    using bignum_ptr = common::intrusive_ptr<bignum>;

    class ecp_point;
	using ecp_point_ptr = common::intrusive_ptr<ecp_point>;

	class ecp_point : public meta::object {
		META_OBJECT
	private:
		mbedtls_ecp_point m_point;
	public:
        ecp_point();
        ~ecp_point();
		
		const mbedtls_ecp_point* get() const { return &m_point;}
        mbedtls_ecp_point* get() { return &m_point;}

		void set_zero();
		bool is_zero();
		bool cmp(const ecp_point_ptr& pnt) const;
		lua::multiret read_string(lua::state& l);
       
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};
	

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

        lua::multiret point_read_binary(lua::state& l);
        lua::multiret point_write_binary(lua::state& l);
		bool check_pubkey(const ecp_point_ptr& pnt) const;
        bool check_privkey(const bignum_ptr& pnt) const;
        lua::multiret ecdsa_verify(lua::state& l);
        lua::multiret ecdsa_sign(lua::state& l);
        lua::multiret gen_keypair(lua::state& l);
        lua::multiret gen_privkey(lua::state& l);
        lua::multiret gen_pubkey(lua::state& l);
        lua::multiret set_random_data(lua::state& l);
		void set_random(const random_ptr& r) { m_random = r; }
		lua::multiret ecdh_gen_public(lua::state& l);
		lua::multiret ecdh_compute_shared(lua::state& l);
        
		lua::multiret scalar_mul(lua::state& l);
		static lua::multiret lnew(lua::state& l);
		static void lbind(lua::state& l);
	};
	using ecp_ptr = common::intrusive_ptr<ecp>;
}
