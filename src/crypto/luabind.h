#pragma once

/// Lua glue for the crypto module.
///
/// The crypto C++ API is Lua-free; this header keeps the historical Lua API:
/// flexible argument forms (algorithm name or crypto.MD_* value, integer or
/// bignum operands, varargs/table data for updates) and multi-value returns.
/// It is included into the generated bindings only (via @luabind_include) and is
/// hidden from the binding parser by the LUABIND_PARSE guard.

#ifndef LUABIND_PARSE

#include "lua/stack.h"
#include "llae/buffer.h"
#include "llae/loop.h"
#include "llae/promise.h"
#include "llae/result.h"
#include "llae/write_buffers.h"

#include "crypto/bignum.h"
#include "crypto/cipher.h"
#include "crypto/crypto.h"
#include "crypto/ecp.h"
#include "crypto/hmac.h"
#include "crypto/md.h"

namespace crypto {

	/// Reads a bignum operand as a bignum (integers go through the overloads).
	inline bignum_ptr lua_bignum_operand(lua::state& l,int idx) {
		auto value = lua::stack<bignum_ptr>::get(l,idx);
		if (!value) {
			l.argerror(idx,"need bignum or integer");
		}
		return value;
	}

	/// Reads the bignum method receiver.
	inline bignum_ptr lua_bignum_self(lua::state& l) {
		auto self = lua::stack<bignum_ptr>::get(l,1);
		if (!self) {
			l.argerror(1,"need bignum");
		}
		return self;
	}

	/// Creates a digest from an algorithm name or a crypto.MD_* value.
	inline llae::result<md_ptr> lua_md_create(lua::state& l) {
		md_ptr res;
		if (l.isnumber(1)) {
			res = md::create(static_cast<mbedtls_md_type_t>(l.tointeger(1)));
		} else if (l.isstring(1)) {
			res = md::create(l.tostring(1));
		}
		if (!res) {
			return llae::string_error::create("unknown md algorithm");
		}
		return llae::result<md_ptr>(std::move(res));
	}

	/// Returns the digest size for an algorithm name or a crypto.MD_* value.
	inline llae::result<size_t> lua_md_get_length(lua::state& l) {
		if (l.isnumber(1)) {
			return md::get_length(static_cast<mbedtls_md_type_t>(l.tointeger(1)));
		}
		if (l.isstring(1)) {
			return md::get_length(std::string_view(l.tostring(1)));
		}
		return llae::string_error::create("unknown md algorithm");
	}

	/// Updates a digest with one or more Lua buffers (varargs or a table).
	inline llae::result_promise_ptr<void> lua_md_update(lua::state& l) {
		auto self = lua::stack<md_ptr>::get(l,1);
		if (!self) {
			l.argerror(1,"need md");
		}
		llae::write_buffers buffers;
		if (!buffers.putm(l,2)) {
			buffers.reset(l);
			return llae::make_result_promise_string_error<void>("invalid data");
		}
		return self->async_update_buffers(llae::loop::get(l),std::move(buffers));
	}

	/// Creates an HMAC from an algorithm name or a crypto.MD_* value.
	inline llae::result<hmac_ptr> lua_hmac_create(lua::state& l) {
		hmac_ptr res;
		if (l.isnumber(1)) {
			res = hmac::create(static_cast<mbedtls_md_type_t>(l.tointeger(1)));
		} else if (l.isstring(1)) {
			res = hmac::create(l.tostring(1));
		}
		if (!res) {
			return llae::string_error::create("unknown hmac algorithm");
		}
		return llae::result<hmac_ptr>(std::move(res));
	}

	/// Updates an HMAC with one or more Lua buffers (varargs or a table).
	inline llae::result_promise_ptr<void> lua_hmac_update(lua::state& l) {
		auto self = lua::stack<hmac_ptr>::get(l,1);
		if (!self) {
			l.argerror(1,"need hmac");
		}
		llae::write_buffers buffers;
		if (!buffers.putm(l,2)) {
			buffers.reset(l);
			return llae::make_result_promise_string_error<void>("invalid data");
		}
		return self->async_update_buffers(llae::loop::get(l),std::move(buffers));
	}

	/// Updates a cipher with one or more Lua buffers (varargs or a table).
	inline llae::result_promise_ptr<llae::buffer_base_ptr> lua_cipher_update(lua::state& l) {
		auto self = lua::stack<cipher_ptr>::get(l,1);
		if (!self) {
			l.argerror(1,"need cipher");
		}
		llae::write_buffers buffers;
		if (!buffers.putm(l,2)) {
			buffers.reset(l);
			return llae::make_result_promise_string_error<llae::buffer_base_ptr>("invalid data");
		}
		return self->async_update_buffers(llae::loop::get(l),std::move(buffers));
	}

	/// HKDF with an algorithm name or a crypto.MD_* value.
	inline llae::result_promise_ptr<llae::buffer_base_ptr> lua_hkdf(lua::state& l) {
		auto& loop = llae::loop::get(l);
		auto salt = lua::stack<llae::buffer_base_ptr>::get(l,2);
		auto info = lua::stack<llae::buffer_base_ptr>::get(l,3);
		auto key = lua::stack<llae::buffer_base_ptr>::get(l,4);
		auto osize = static_cast<size_t>(l.checkinteger(5));
		if (l.isnumber(1)) {
			return async_hkdf(loop,static_cast<mbedtls_md_type_t>(l.tointeger(1)),
				std::move(salt),std::move(info),std::move(key),osize);
		}
		if (l.isstring(1)) {
			return async_hkdf(loop,std::string_view(l.tostring(1)),
				std::move(salt),std::move(info),std::move(key),osize);
		}
		return llae::make_result_promise_string_error<llae::buffer_base_ptr>("unknown md algorithm");
	}

	/// Creates a bignum from nothing, an integer, a string (with an optional
	/// radix) or another bignum.
	inline llae::result<bignum_ptr> lua_bignum_new(lua::state& l) {
		if (l.isnoneornil(1)) {
			return bignum::create(std::nullopt);
		}
		if (l.isnumber(1)) {
			return bignum::create(static_cast<int64_t>(l.checkinteger(1)));
		}
		if (l.isstring(1)) {
			auto radix = static_cast<int>(l.optinteger(2,16));
			return bignum::from_string(std::string_view(l.tostring(1)),radix);
		}
		auto other = lua::stack<bignum_ptr>::get(l,1);
		if (!other) {
			l.argerror(1,"need bignum, integer or string");
		}
		auto res = bignum::create(std::nullopt);
		if (res.get_error()) {
			return res.get_error();
		}
		res.get_result()->set(*other);
		return res;
	}

	/// Adds another big number to this one and returns the result.
	inline llae::result<bignum_ptr> lua_bignum_add(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->add(l.checkinteger(2));
		}
		return self->add(lua_bignum_operand(l,2));
	}

	/// Subtracts another big number from this one and returns the result.
	inline llae::result<bignum_ptr> lua_bignum_sub(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->sub(l.checkinteger(2));
		}
		return self->sub(lua_bignum_operand(l,2));
	}

	/// Multiplies this big number by another and returns the result.
	inline llae::result<bignum_ptr> lua_bignum_mul(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->mul(l.checkinteger(2));
		}
		return self->mul(lua_bignum_operand(l,2));
	}

	/// Divides this big number by another and returns quotient and remainder.
	inline llae::result<bignum_div_result> lua_bignum_div(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->div(l.checkinteger(2));
		}
		return self->div(lua_bignum_operand(l,2));
	}

	/// Returns the remainder of the division by another big number.
	inline llae::result<bignum_ptr> lua_bignum_mod(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->mod(l.checkinteger(2));
		}
		return self->mod(lua_bignum_operand(l,2));
	}

	/// Adds another big number to this one (in-place).
	inline llae::result<void> lua_bignum_self_add(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->self_add(l.checkinteger(2));
		}
		return self->self_add(lua_bignum_operand(l,2));
	}

	/// Subtracts another big number from this one (in-place).
	inline llae::result<void> lua_bignum_self_sub(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->self_sub(l.checkinteger(2));
		}
		return self->self_sub(lua_bignum_operand(l,2));
	}

	/// Multiplies this big number by another (in-place).
	inline llae::result<void> lua_bignum_self_mul(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->self_mul(l.checkinteger(2));
		}
		return self->self_mul(lua_bignum_operand(l,2));
	}

	/// Checks if this big number is less than another.
	inline bool lua_bignum_less(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->less(l.checkinteger(2));
		}
		return self->less(lua_bignum_operand(l,2));
	}

	/// Checks if this big number is less than or equal to another.
	inline bool lua_bignum_lequal(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->lequal(l.checkinteger(2));
		}
		return self->lequal(lua_bignum_operand(l,2));
	}

	/// Checks if this big number is equal to another.
	inline bool lua_bignum_equal(lua::state& l) {
		auto self = lua_bignum_self(l);
		if (l.isnumber(2)) {
			return self->equal(l.checkinteger(2));
		}
		return self->equal(lua_bignum_operand(l,2));
	}

}

namespace lua {

	/// Pushes the quotient and the remainder of a bignum division.
	template <>
	struct stack<crypto::bignum_div_result> {
		static int push(state& s,const crypto::bignum_div_result& result) {
			int res = lua::push(s,result.quotient);
			res += lua::push(s,result.remainder);
			return res;
		}
	};

	/// Pushes the private and the public key of an elliptic curve key pair.
	template <>
	struct stack<crypto::ecp_keypair> {
		static int push(state& s,const crypto::ecp_keypair& keypair) {
			int res = lua::push(s,keypair.private_key);
			res += lua::push(s,keypair.public_key);
			return res;
		}
	};

	/// Pushes the r and the s components of an ECDSA signature.
	template <>
	struct stack<crypto::ecdsa_signature> {
		static int push(state& s,const crypto::ecdsa_signature& signature) {
			int res = lua::push(s,signature.r);
			res += lua::push(s,signature.s);
			return res;
		}
	};

}

#endif /* LUABIND_PARSE */
