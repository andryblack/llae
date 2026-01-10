#ifndef __LLAE_CRYPTO_BIGNUM_H_INCLUDED__
#define __LLAE_CRYPTO_BIGNUM_H_INCLUDED__

#include "llae-private/mbedtls/bignum.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {
    
    class bignum;
    using bignum_ptr = common::intrusive_ptr<bignum>;
    
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

        bool is0() const;
        bool get_bit(size_t bit) const;
        void set_bit(size_t bit, bool value);
        void lset_bit(lua::state& l);

        void self_add(lua::state& l);
        void self_mul(lua::state& l);
        void self_sub(lua::state& l);
        bignum_ptr mul(lua::state& l);
        lua::multiret div(lua::state& l);
        bignum_ptr add(lua::state& l);
        bignum_ptr sub(lua::state& l);
        bignum_ptr mod(lua::state& l);
        bignum_ptr exp_mod(lua::state& l);
        void self_lshift(lua::state& l);
        void self_rshift(lua::state& l);
        lua::multiret write(lua::state& l);
        lua::multiret write_le(lua::state& l);
        lua::multiret read(lua::state& l);
        lua::multiret read_le(lua::state& l);

        lua::multiret less(lua::state& l);
        lua::multiret lequal(lua::state& l);
        lua::multiret equal(lua::state& l);
        
        lua::multiret tostring(lua::state& l);
        
        static lua::multiret lnew(lua::state& l);
        static lua::multiret lrandom(lua::state& l);
        static void lbind(lua::state& l);
    };

}

#endif /*__LLAE_CRYPTO_BIGNUM_H_INCLUDED__*/

