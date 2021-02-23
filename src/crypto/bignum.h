#ifndef __LLAE_CRYPTO_BIGNUM_H_INCLUDED__
#define __LLAE_CRYPTO_BIGNUM_H_INCLUDED__

#include <mbedtls/bignum.h>
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
        
        void set(mbedtls_mpi_sint val);
        void set(const bignum& val);

        void self_add(lua::state& l);
        void self_mul(lua::state& l);
        void self_sub(lua::state& l);
        bignum_ptr mul(lua::state& l);
        bignum_ptr add(lua::state& l);
        bignum_ptr sub(lua::state& l);
        lua::multiret write(lua::state& l);
        lua::multiret read(lua::state& l);

        bool less(const bignum_ptr& b) const;
        bool lequal(const bignum_ptr& b) const;
        
        lua::multiret tostring(lua::state& l);
        
        static lua::multiret lnew(lua::state& l);
        static void lbind(lua::state& l);
    };

}

#endif /*__LLAE_CRYPTO_BIGNUM_H_INCLUDED__*/

