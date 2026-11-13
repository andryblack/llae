-- Lua API compatibility checks for the crypto module: the C++ API is
-- Lua-free, src/crypto/luabind.h restores the historical Lua forms.
local lu = require 'luaunit'
local llae = require 'llae'
local crypto = require 'llae.crypto'

testCryptoCompat = {}

local function hex(b)
	return tostring(llae.buffer.hex_encode(b))
end

local function digest(data)
	local md = assert(crypto.md.new('SHA256'))
	assert(md:update(data))
	return hex(assert(md:finish()))
end

-- Message digest/HMAC factories accept both names and crypto.MD_* values.

function testCryptoCompat:test_md_new_by_type()
	local by_name = assert(crypto.md.new('SHA256'))
	assert(by_name:update('abc'))
	local a = hex(assert(by_name:finish()))
	local by_type = assert(crypto.md.new(crypto.MD_SHA256))
	assert(by_type:update('abc'))
	local b = hex(assert(by_type:finish()))
	lu.assertEquals(a,b)
	lu.assertEquals(a,'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad')
end

function testCryptoCompat:test_md_new_errors()
	local md,err = crypto.md.new('UNKNOWN')
	lu.assertNil(md)
	lu.assertNotNil(err)
	local md2,err2 = crypto.md.new({})
	lu.assertNil(md2)
	lu.assertNotNil(err2)
end

function testCryptoCompat:test_md_get_length()
	lu.assertEquals(crypto.md.get_length('SHA256'),32)
	lu.assertEquals(crypto.md.get_length(crypto.MD_SHA256),32)
	lu.assertNil(crypto.md.get_length('UNKNOWN'))
end

function testCryptoCompat:test_hmac_new_by_type()
	local by_name = assert(crypto.hmac.new('SHA256'))
	assert(by_name:start('key'))
	assert(by_name:update('abc'))
	local a = hex(assert(by_name:finish()))
	local by_type = assert(crypto.hmac.new(crypto.MD_SHA256))
	assert(by_type:start('key'))
	assert(by_type:update('abc'))
	local b = hex(assert(by_type:finish()))
	lu.assertEquals(a,b)
end

function testCryptoCompat:test_hmac_new_errors()
	local hmac,err = crypto.hmac.new('UNKNOWN')
	lu.assertNil(hmac)
	lu.assertNotNil(err)
	local hmac2,err2 = crypto.hmac.new(9999)
	lu.assertNil(hmac2)
	lu.assertNotNil(err2)
end

-- update() accepts several buffers (varargs) or a table of buffers.

function testCryptoCompat:test_md_update_varargs()
	lu.assertEquals(digest('Hello, World!'),digest({'Hello, ','World!'}))
	local md = assert(crypto.md.new('SHA256'))
	assert(md:update('Hello, ','World','!'))
	lu.assertEquals(hex(assert(md:finish())),digest('Hello, World!'))
end

function testCryptoCompat:test_hmac_update_varargs()
	local whole = assert(crypto.hmac.new('SHA256'))
	assert(whole:start('key'))
	assert(whole:update('Hello, World!'))
	local a = hex(assert(whole:finish()))

	local parts = assert(crypto.hmac.new('SHA256'))
	assert(parts:start('key'))
	assert(parts:update('Hello, '))
	assert(parts:update({'World','!'}))
	lu.assertEquals(hex(assert(parts:finish())),a)
end

function testCryptoCompat:test_cipher_update_varargs()
	local key = '0123456789abcdef0123456789abcdef'
	local iv = '0123456789abcdef'
	local data = 'The quick brown fox jumps over the lazy dog!!'
	local function encrypt(chunks)
		local cipher = assert(crypto.cipher.new('AES-256-CBC'))
		assert(cipher:set_key(key,crypto.ENCRYPT))
		assert(cipher:set_iv(iv))
		assert(cipher:set_padding(crypto.PADDING_PKCS7))
		local out = tostring(assert(cipher:update(chunks))) .. tostring(assert(cipher:finish()))
		return tostring(llae.buffer.hex_encode(out))
	end
	lu.assertEquals(encrypt(data),encrypt({'The quick ','brown fox ','jumps over the lazy dog!!'}))
end

function testCryptoCompat:test_cipher_new_alias()
	local cipher = assert(crypto.cipher.new('AES-128-CBC'))
	lu.assertEquals(cipher:get_block_size(),16)
end

-- bignum arithmetic accepts integers and bignums and returns the old shapes.

function testCryptoCompat:test_bignum_new_forms()
	lu.assertEquals(tostring(assert(crypto.bignum.new(10))),'0A')
	lu.assertEquals(tostring(assert(crypto.bignum.new('ff',16))),'FF')
	lu.assertEquals(tostring(assert(crypto.bignum.new('ff'))),'FF')
	local copy = assert(crypto.bignum.new(assert(crypto.bignum.new(42))))
	lu.assertEquals(tostring(copy),'2A')
	lu.assertEquals(tostring(assert(crypto.bignum.new())),'')
end

function testCryptoCompat:test_bignum_number_operands()
	local a = assert(crypto.bignum.new(10))
	lu.assertEquals(tostring(assert(a:add(5))),'0F')
	lu.assertEquals(tostring(assert(a + 5)),'0F')
	lu.assertEquals(tostring(assert(a - 1)),'09')
	lu.assertEquals(tostring(assert(a * 2)),'14')
	lu.assertEquals(tostring(assert(a:mod(3))),'01')
	lu.assertEquals(tostring(assert(a:exp_mod(crypto.bignum.new(2),crypto.bignum.new(7)))),'02')
end

function testCryptoCompat:test_bignum_div_multiret()
	local a = assert(crypto.bignum.new(10))
	local quotient,remainder = a:div(3)
	lu.assertEquals(tostring(quotient),'03')
	lu.assertEquals(tostring(remainder),'01')
	-- Lua arithmetic metamethods return a single value, as in the old API.
	lu.assertEquals(tostring(a / 3),'03')
end

function testCryptoCompat:test_bignum_self_ops()
	local a = assert(crypto.bignum.new(10))
	assert(a:self_add(5))
	lu.assertEquals(tostring(a),'0F')
	assert(a:self_sub(3))
	lu.assertEquals(tostring(a),'0C')
	assert(a:self_mul(2))
	lu.assertEquals(tostring(a),'18')
end

function testCryptoCompat:test_bignum_compare()
	local a = assert(crypto.bignum.new(10))
	lu.assertTrue(a:equal(10))
	lu.assertFalse(a:equal(11))
	lu.assertTrue(a == crypto.bignum.new(10))
	lu.assertFalse(a == crypto.bignum.new(11))
	-- Lua does not call __eq for mixed userdata/number values.
	lu.assertFalse(a == 10)
end

function testCryptoCompat:test_bignum_errors()
	local a = assert(crypto.bignum.new(10))
	-- Bad operands raise, as the old lua::multiret implementation did.
	lu.assertErrorMsgContains('need bignum',function()
		return a:add('nope')
	end)
	lu.assertErrorMsgContains('need bignum',function()
		return a:add({})
	end)
end

-- hkdf accepts both names and crypto.MD_* values.

function testCryptoCompat:test_hkdf_by_type()
	local salt = '000102030405060708090a0b0c'
	local info = 'f0f1f2f3f4f5f6f7f8f9'
	local key = '0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b'
	local a = assert(crypto.hkdf('SHA256',salt,info,key,42))
	local b = assert(crypto.hkdf(crypto.MD_SHA256,salt,info,key,42))
	lu.assertEquals(hex(a),hex(b))
end

-- ecp multi-value results are returned as multiple Lua values.

function testCryptoCompat:test_ecp_gen_keypair_multiret()
	local ecp = assert(crypto.ecp.new('secp256r1'))
	local privkey,pubkey = ecp:gen_keypair()
	lu.assertNotNil(privkey)
	lu.assertNotNil(pubkey)
	lu.assertTrue(ecp:check_privkey(privkey))
	lu.assertTrue(ecp:check_pubkey(pubkey))
end

function testCryptoCompat:test_ecp_ecdsa_multiret()
	local ecp = assert(crypto.ecp.new('secp256r1'))
	local privkey,pubkey = ecp:gen_keypair()
	local hash = assert(crypto.sha256sum('Hello, World!'))
	local r,s = ecp:ecdsa_sign(hash,privkey)
	lu.assertNotNil(r)
	lu.assertNotNil(s)
	lu.assertTrue(ecp:ecdsa_verify(hash,pubkey,r,s))
end

function testCryptoCompat:test_ecp_ecdh_multiret()
	local ecp1 = assert(crypto.ecp.new('secp256r1'))
	local ecp2 = assert(crypto.ecp.new('secp256r1'))
	local priv1,pub1 = ecp1:ecdh_gen_public()
	local priv2,pub2 = ecp2:ecdh_gen_public()
	lu.assertNotNil(priv1)
	lu.assertNotNil(pub1)
	local shared1 = assert(ecp1:ecdh_compute_shared(pub2,priv1))
	local shared2 = assert(ecp2:ecdh_compute_shared(pub1,priv2))
	lu.assertEquals(hex(assert(shared1:write())),hex(assert(shared2:write())))
end

function testCryptoCompat:test_hkdf_errors()
	local key,err = crypto.hkdf('UNKNOWN','salt','info','key',16)
	lu.assertNil(key)
	lu.assertNotNil(err)
end

-- random and entropy keep their constructors and method forms.

function testCryptoCompat:test_random_entropy()
	local entropy = assert(crypto.entropy.new())
	assert(entropy:update_manual('some entropy'))
	local rng = assert(crypto.random.new(entropy))
	assert(rng:update('more entropy'))
	assert(rng:seed('personalization'))
	local value = assert(crypto.bignum.random(rng,8))
	lu.assertEquals(#tostring(assert(value:write())),8)

	local rng2 = assert(crypto.random.new())
	assert(rng2:seed())
end

function testCryptoCompat:test_bignum_division_by_zero()
	-- Status errors are returned as (nil, error) instead of being raised.
	local a = assert(crypto.bignum.new(10))
	local quotient,err = a:div(0)
	lu.assertNil(quotient)
	lu.assertNotNil(err)
	local remainder,err2 = a:mod(0)
	lu.assertNil(remainder)
	lu.assertNotNil(err2)
end

function testCryptoCompat:test_bignum_integer_overloads()
	-- Integer operands use the C++ integer overloads (no temporary bignum).
	local a = assert(crypto.bignum.new(10))
	lu.assertEquals(tostring(a:add(5)),tostring(a:add(crypto.bignum.new(5))))
	lu.assertEquals(tostring(a:sub(5)),tostring(a:sub(crypto.bignum.new(5))))
	lu.assertEquals(tostring(a:mul(5)),tostring(a:mul(crypto.bignum.new(5))))
	lu.assertEquals(tostring(a:mod(3)),tostring(a:mod(crypto.bignum.new(3))))
	local q1,r1 = a:div(3)
	local q2,r2 = a:div(crypto.bignum.new(3))
	lu.assertEquals(tostring(q1),tostring(q2))
	lu.assertEquals(tostring(r1),tostring(r2))

	-- 64-bit operands (mbedtls_mpi_*_int takes mbedtls_mpi_sint = int64_t)
	local big = 0x100000000
	lu.assertEquals(tostring(a:add(big)),tostring(a:add(crypto.bignum.new(big))))
	lu.assertEquals(tostring(a:add(big)),'010000000A')

	-- Negative integers (mbedtls_mpi_mul_int is unsigned, the sign is restored)
	lu.assertEquals(tostring(a:mul(-3)),tostring(a:mul(crypto.bignum.new(-3))))
	lu.assertEquals(tostring(a:mul(-3)),'-1E')

	local b = assert(crypto.bignum.new(10))
	assert(b:self_add(5))
	lu.assertEquals(tostring(b),'0F')
	assert(b:self_mul(-2))
	lu.assertEquals(tostring(b),'-1E')

	lu.assertTrue(a:equal(10))
	lu.assertFalse(a:equal(11))
end

function testCryptoCompat:test_bignum_fractional_operands()
	local a = assert(crypto.bignum.new(10))
	-- An integral float is accepted (Lua's integer representation rules).
	lu.assertEquals(tostring(a:add(2.0)),'0C')
	-- A fractional number is rejected instead of silently becoming 0.
	lu.assertErrorMsgContains('no integer representation',function() return a:add(2.5) end)
	lu.assertErrorMsgContains('no integer representation',function() return a:mul(2.5) end)
	lu.assertErrorMsgContains('no integer representation',function() return a:div(2.5) end)
	lu.assertErrorMsgContains('no integer representation',function() return crypto.bignum.new(2.5) end)
end
