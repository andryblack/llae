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
    using bignum_ptr = common::intrusive_ptr<bignum>;

    class ecp_point;
	using ecp_point_ptr = common::intrusive_ptr<ecp_point>;

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

		/// @luabind
		void set_zero();
		/// @luabind
		bool is_zero();
		/// @luabind
		bool cmp(const ecp_point_ptr& pnt) const;
		/// @luabind
		lua::multiret read_string(lua::state& l);
       
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};
	

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

		/// @luabind
        lua::multiret point_read_binary(lua::state& l);
		/// @luabind
        lua::multiret point_write_binary(lua::state& l);
		/// @luabind
		bool check_pubkey(const ecp_point_ptr& pnt) const;
		/// @luabind
        bool check_privkey(const bignum_ptr& pnt) const;
		/// @luabind
        lua::multiret ecdsa_verify(lua::state& l);
		/// @luabind
        lua::multiret ecdsa_sign(lua::state& l);
		/// @luabind
        lua::multiret gen_keypair(lua::state& l);
		/// @luabind
        lua::multiret gen_privkey(lua::state& l);
		/// @luabind
        lua::multiret gen_pubkey(lua::state& l);
		/// @luabind
        void set_random_data(const llae::buffer_view& data);
		/// @luabind
		void set_random(const random_ptr& r) { m_random = r; }
		/// @luabind
		lua::multiret ecdh_gen_public(lua::state& l);
		/// @luabind
		lua::multiret ecdh_compute_shared(lua::state& l);
		/// @luabind
        lua::multiret scalar_mul(lua::state& l);

		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
	};
	using ecp_ptr = common::intrusive_ptr<ecp>;
}
