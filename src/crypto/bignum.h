#ifndef __LLAE_CRYPTO_BIGNUM_H_INCLUDED__
#define __LLAE_CRYPTO_BIGNUM_H_INCLUDED__

#include "llae-private/mbedtls/bignum.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {
    
    class bignum;
    using bignum_ptr = common::intrusive_ptr<bignum>;
    
    /// @luabind
    class bignum : public meta::object {
        META_OBJECT
    private:
        mbedtls_mpi m_mpi;
    public:
        bignum();
        ~bignum();
        
        const mbedtls_mpi* get() const { return &m_mpi;}
        mbedtls_mpi* get() { return &m_mpi;}
        
        void set(mbedtls_mpi_sint val);
        void set(const bignum& val);

        /// @luabind
        bool is0() const;
        /// @luabind
        bool get_bit(size_t bit) const;
        void set_bit(size_t bit, bool value);
        /// @luabind(name=set_bit)
        void lset_bit(lua::state& l);

        /// @luabind
        void self_add(lua::state& l);
        /// @luabind
        void self_mul(lua::state& l);
        /// @luabind
        void self_sub(lua::state& l);
        /// @luabind(alias=__mul)
        bignum_ptr mul(lua::state& l);
        /// @luabind(alias=__div)
        lua::multiret div(lua::state& l);
        /// @luabind(alias=__add)
        bignum_ptr add(lua::state& l);
        /// @luabind(alias=__sub)
        bignum_ptr sub(lua::state& l);
        /// @luabind
        bignum_ptr mod(lua::state& l);
        /// @luabind
        bignum_ptr exp_mod(lua::state& l);
        /// @luabind
        void self_lshift(lua::state& l);
        /// @luabind
        void self_rshift(lua::state& l);
        /// @luabind
        lua::multiret write(lua::state& l);
        /// @luabind
        lua::multiret write_le(lua::state& l);
        /// @luabind
        lua::multiret read(lua::state& l);
        /// @luabind
        lua::multiret read_le(lua::state& l);

        /// @luabind(name=__lt)
        lua::multiret less(lua::state& l);
        /// @luabind(name=__le)
        lua::multiret lequal(lua::state& l);
        /// @luabind(alias=__eq)
        lua::multiret equal(lua::state& l);
        
        /// @luabind(alias=__tostring)
        lua::multiret tostring(lua::state& l);
        
        /// @luabind(name=new)
        static lua::multiret lnew(lua::state& l);
        /// @luabind(name=random)
        static lua::multiret lrandom(lua::state& l);
    };

}

#endif /*__LLAE_CRYPTO_BIGNUM_H_INCLUDED__*/
