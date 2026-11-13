#ifndef __LLAE_CRYPTO_BIGNUM_H_INCLUDED__
#define __LLAE_CRYPTO_BIGNUM_H_INCLUDED__

#include "llae-private/mbedtls/bignum.h"
#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "llae/buffer.h"
#include "llae/result.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace crypto {

	class bignum;
	class random;
	using random_ptr = common::intrusive_ptr<random>;

	/// @luabind
	using bignum_ptr = common::intrusive_ptr<bignum>;

	/// Quotient and remainder of a bignum division.
	struct bignum_div_result {
		bignum_ptr quotient;
		bignum_ptr remainder;
	};

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

		/// Creates a new big number instance, optionally initialized with an integer or a bignum.
		/// @lparam(value,integer|string|crypto.bignum?) The initial value
		/// @lparam(radix,integer?) The radix for a string value (16 by default)
		/// @lreturn(result,crypto.bignum)
		/// @lreturn(error,string?)
		/// @luabind(name=new,wrapper=crypto::lua_bignum_new)
		static llae::result<bignum_ptr> create(std::optional<int64_t> value);

		/// Creates a new big number instance from a string representation.
		static llae::result<bignum_ptr> from_string(std::string_view value, std::optional<int> radix);

		/// Creates a new big number instance filled with random data.
		/// @lparam(rng,crypto.random) The random number generator
		/// @lparam(len,integer) The number of random bytes
		/// @lreturn(result,crypto.bignum)
		/// @lreturn(error,string?)
		/// @luabind(name=random)
		static llae::result<bignum_ptr> create_random(const random_ptr& rng, size_t len);

		/// @luabind
		bool is0() const;
		/// @luabind
		bool get_bit(size_t bit) const;
		/// Sets a bit of this big number.
		/// @lparam(bit,integer) The bit index
		/// @lparam(value,boolean) The bit value
		/// @luabind(name=set_bit)
		llae::result<void> set_bit(size_t bit, bool value);

		/// Adds another big number to this one (in-place).
		/// @lparam(other,crypto.bignum|integer) The big number to add
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind(wrapper=crypto::lua_bignum_self_add)
		llae::result<void> self_add(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<void> self_add(int64_t other);
		/// Multiplies this big number by another (in-place).
		/// @lparam(other,crypto.bignum|integer) The big number to multiply by
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind(wrapper=crypto::lua_bignum_self_mul)
		llae::result<void> self_mul(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<void> self_mul(int64_t other);
		/// Subtracts another big number from this one (in-place).
		/// @lparam(other,crypto.bignum|integer) The big number to subtract
		/// @lreturn(result,boolean?)
		/// @lreturn(error,string?)
		/// @luabind(wrapper=crypto::lua_bignum_self_sub)
		llae::result<void> self_sub(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<void> self_sub(int64_t other);
		/// Shifts this big number left (in-place).
		/// @lparam(shift,integer) The number of bits to shift
		/// @luabind
		llae::result<void> self_lshift(size_t shift);
		/// Shifts this big number right (in-place).
		/// @lparam(shift,integer) The number of bits to shift
		/// @luabind
		llae::result<void> self_rshift(size_t shift);
		/// Multiplies this big number by another and returns the result.
		/// @lparam(other,crypto.bignum|integer) The big number to multiply by
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind(alias=__mul,wrapper=crypto::lua_bignum_mul)
		llae::result<bignum_ptr> mul(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<bignum_ptr> mul(int64_t other);
		/// Divides this big number by another.
		/// @lparam(other,crypto.bignum|integer) The divisor
		/// @lreturn(quotient,crypto.bignum?)
		/// @lreturn(remainder,crypto.bignum?)
		/// @luabind(alias=__div,wrapper=crypto::lua_bignum_div)
		llae::result<bignum_div_result> div(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<bignum_div_result> div(int64_t other);
		/// Adds another big number to this one and returns the result.
		/// @lparam(other,crypto.bignum|integer) The big number to add
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind(alias=__add,wrapper=crypto::lua_bignum_add)
		llae::result<bignum_ptr> add(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<bignum_ptr> add(int64_t other);
		/// Subtracts another big number from this one and returns the result.
		/// @lparam(other,crypto.bignum|integer) The big number to subtract
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind(alias=__sub,wrapper=crypto::lua_bignum_sub)
		llae::result<bignum_ptr> sub(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<bignum_ptr> sub(int64_t other);
		/// @lparam(other,crypto.bignum|integer) The divisor
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind(wrapper=crypto::lua_bignum_mod)
		llae::result<bignum_ptr> mod(const bignum_ptr& other);
		/// Integer overload, avoids a temporary bignum.
		llae::result<bignum_ptr> mod(int64_t other);
		/// @lparam(e,crypto.bignum) The exponent
		/// @lparam(n,crypto.bignum) The modulus
		/// @lparam(supp,crypto.bignum?) Optional precomputed value
		/// @lreturn(result,crypto.bignum?)
		/// @lreturn(error,string?)
		/// @luabind
		llae::result<bignum_ptr> exp_mod(const bignum_ptr& e, const bignum_ptr& n, const bignum_ptr& supp);

		/// Writes the big number to a string representation.
		/// @lparam(radix,integer?) The radix of the representation (16 by default)
		/// @luabind(name=tostring,alias=__tostring)
		llae::result<std::string> to_string(std::optional<int> radix) const;
		/// Writes the big number to a big-endian binary buffer.
		/// @lparam(len,integer?) The output buffer size
		/// @luabind
		llae::result<llae::buffer_base_ptr> write(std::optional<size_t> len) const;
		/// Writes the big number to a little-endian binary buffer.
		/// @lparam(len,integer?) The output buffer size
		/// @luabind
		llae::result<llae::buffer_base_ptr> write_le(std::optional<size_t> len) const;
		/// Reads a big number from big-endian binary data.
		/// @lparam(data,string|llae.buffer_base) The input data
		/// @luabind
		llae::result<void> read(const llae::buffer_view& data);
		/// Reads a big number from little-endian binary data.
		/// @lparam(data,string|llae.buffer_base) The input data
		/// @luabind
		llae::result<void> read_le(const llae::buffer_view& data);

		/// Checks if this big number is less than another.
		/// @lparam(other,crypto.bignum|integer) The big number to compare with
		/// @lreturn(result,boolean)
		/// @luabind(name=__lt,wrapper=crypto::lua_bignum_less)
		bool less(const bignum_ptr& other) const;
		/// Integer overload.
		bool less(int64_t other) const;
		/// Checks if this big number is less than or equal to another.
		/// @lparam(other,crypto.bignum|integer) The big number to compare with
		/// @lreturn(result,boolean)
		/// @luabind(name=__le,wrapper=crypto::lua_bignum_lequal)
		bool lequal(const bignum_ptr& other) const;
		/// Integer overload.
		bool lequal(int64_t other) const;
		/// Checks if this big number is equal to another.
		/// @lparam(other,crypto.bignum|integer) The big number to compare with
		/// @lreturn(result,boolean)
		/// @luabind(alias=__eq,wrapper=crypto::lua_bignum_equal)
		bool equal(const bignum_ptr& other) const;
		/// Integer overload.
		bool equal(int64_t other) const;
	};

}

#endif /*__LLAE_CRYPTO_BIGNUM_H_INCLUDED__*/
