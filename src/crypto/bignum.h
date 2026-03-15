#ifndef __LLAE_CRYPTO_BIGNUM_H_INCLUDED__
#define __LLAE_CRYPTO_BIGNUM_H_INCLUDED__

#include "llae-private/mbedtls/bignum.h"
#include "meta/object.h"
#include "lua/state.h"
#include "common/intrusive_ptr.h"

namespace crypto {
    
    class bignum;
    /// @luabind
    using bignum_ptr = common::intrusive_ptr<bignum>;
    
    /**
    * Big number arithmetic operations for cryptographic computations.
    */
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

        /// Adds another big number to this one (in-place).
        /// @lparam(other,crypto.bignum) The big number to add
        /// @luabind
        void self_add(lua::state& l);
        /// Multiplies this big number by another (in-place).
        /// @lparam(other,crypto.bignum) The big number to multiply by
        /// @luabind
        void self_mul(lua::state& l);
        /// Subtracts another big number from this one (in-place).
        /// @lparam(other,crypto.bignum) The big number to subtract
        /// @luabind
        void self_sub(lua::state& l);
        /// Multiplies this big number by another and returns the result.
        /// @lparam(other,crypto.bignum) The big number to multiply by
        /// @luabind(alias=__mul)
        bignum_ptr mul(lua::state& l);
        /// Divides this big number by another.
        /// @lparam(other,crypto.bignum) The divisor
        /// @lreturn(quotient,crypto.bignum?)
        /// @lreturn(remainder,crypto.bignum?)
        /// @lreturn(error,string?)
        /// @luabind(alias=__div)
        lua::multiret div(lua::state& l);
        /// Adds another big number to this one and returns the result.
        /// @lparam(other,crypto.bignum) The big number to add
        /// @luabind(alias=__add)
        bignum_ptr add(lua::state& l);
        /// Subtracts another big number from this one and returns the result.
        /// @lparam(other,crypto.bignum) The big number to subtract
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
        /// Writes the big number to a string representation.
        /// @lparam(len,number?) The output buffer size
        /// @lreturn(result,string?)
        /// @lreturn(error,string?)
        /// @luabind
        lua::multiret write(lua::state& l);
        /// @luabind
        lua::multiret write_le(lua::state& l);
        /// Reads a big number from binary data.
        /// @lparam(data,string|llae.buffer_base) The input data
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        /// @luabind
        lua::multiret read(lua::state& l);
        /// @luabind
        lua::multiret read_le(lua::state& l);

        /// Checks if this big number is less than another.
        /// @lparam(other,crypto.bignum) The big number to compare with
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        /// @luabind(name=__lt)
        lua::multiret less(lua::state& l);
        /// Checks if this big number is less than or equal to another.
        /// @lparam(other,crypto.bignum) The big number to compare with
        /// @lreturn(result,boolean?)
        /// @lreturn(error,string?)
        /// @luabind(name=__le)
        lua::multiret lequal(lua::state& l);
        /// @luabind(alias=__eq)
        lua::multiret equal(lua::state& l);
        
        /// Converts the big number to a string representation.
        /// @lreturn(result,string?)
        /// @lreturn(error,string?)
        /// @luabind(alias=__tostring)
        lua::multiret tostring(lua::state& l);
        
        /// Creates a new big number instance.
        /// @lreturn(result,crypto.bignum)
        /// @luabind(name=new)
        static lua::multiret lnew(lua::state& l);
        /// @luabind(name=random)
        static lua::multiret lrandom(lua::state& l);
    };

}

#endif /*__LLAE_CRYPTO_BIGNUM_H_INCLUDED__*/
