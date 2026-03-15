local fs = require 'llae.fs'
local lu = require 'luaunit'
local llae = require 'llae'
local crypto = require 'llae.crypto'

testCrypto = {}

-- Test RSA public key (RSA 2048-bit)
-- Note: PEM format requires final newline and null terminator for mbedtls
local rsa_public_key_pem = [[-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtFjhy++uZG5ea9ETEeik
bM7r+E8XZaK7ffaKfYLMEledKwBpxbTaxNKJr8eM00mtfkzRDRhk0v5zMUBKM6pt
tHl2EbYfuMESNhuZSh7sg564127HOh2Z0AToAYYU2NksWmg0Xu+L5qzT9iXQXabZ
KC3UokhciHIrd+dtTmYFYmNIafkpasS651Tamv9pdrgjI3pX7lQwQwiQ/vxwNvlP
U5wrMlfy1ozsiEx5iM7K1fxEW9HsD23jc5OB+thktTzsAq4fPoJUVDlt3Hd8Tsol
iwefBCyoA5zFYIBhVTY2HbRCZ+7NuMOm3MjsuVgDAMcUSj3heuNB/JVXXWEA84Eq
oQIDAQAB
-----END PUBLIC KEY-----
]] .. '\0'

local function test_md(data,expected,func)
	local encData = tostring(llae.buffer.hex_decode(data))
	local res = assert(func(encData))
	lu.assertEquals(tostring(llae.buffer.hex_encode(res)),expected)
end

local function test_md_s(data,expected,func)
	local res = assert(func(data))
	lu.assertEquals(tostring(llae.buffer.hex_encode(res)),expected)
end

function testCrypto:test_md5()
	test_md_s('','d41d8cd98f00b204e9800998ecf8427e',crypto.md5sum)
	test_md_s('a','0cc175b9c0f1b6a831c399e269772661',crypto.md5sum)
	test_md_s('abcdefghijklmnopqrstuvwxyz','c3fcd3d76192e4007dfb496cca67e13b',crypto.md5sum)
end

function testCrypto:test_sha256()
	test_md_s('','e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',crypto.sha256sum)
	test_md_s('abc','ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad',crypto.sha256sum)
	test_md_s('abcdefghijklmnopqrstuvwxyz','71c480df93d6ae2f1efad1447c66c9525e316218cf51fc8d9ed832f2daf18b73',crypto.sha256sum)
end

function testCrypto:test_md_concurent()
	local md = assert(crypto.md.new('MD5'))
	local a = md:async_update('test')
	lu.assertNotIsNil(a)
	do
		local r,e = md:update('test1')
		lu.assertIsNil(r)
		lu.assertEquals(tostring(e),'[llae]:failed start update, update is active')
	end
end

function testCrypto:test_crc32()
	-- Test CRC32 checksum calculation
	-- CRC32 values verified with Python's zlib.crc32
	
	-- Empty string
	local crc1 = assert(crypto.crc32(0, ''))
	lu.assertEquals(crc1, 0x0)
	
	-- "Hello, World!"
	local crc2 = assert(crypto.crc32(0, 'Hello, World!'))
	lu.assertEquals(crc2, 0xec4ac3d0)
	
	-- "test"
	local crc3 = assert(crypto.crc32(0, 'test'))
	lu.assertEquals(crc3, 0xd87f7e0c)
	
	-- "123456789" (standard test vector)
	local crc4 = assert(crypto.crc32(0, '123456789'))
	lu.assertEquals(crc4, 0xcbf43926)
end

function testCrypto:test_crc32_incremental()
	-- Test incremental CRC32 calculation
	-- Split "Hello, World!" into parts
	local crc = assert(crypto.crc32(0, 'Hello, '))
	crc = assert(crypto.crc32(crc, 'World!'))
	
	-- Should match full string CRC32
	lu.assertEquals(crc, 0xec4ac3d0)
end

function testCrypto:test_hkdf1()
	-- Test Case 1
	local salt_str = '000102030405060708090a0b0c'
	local info_str = 'f0f1f2f3f4f5f6f7f8f9'
	local key_str = '0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b'

	local salt = llae.buffer.hex_decode(salt_str)
	local info = llae.buffer.hex_decode(info_str)
	local key = llae.buffer.hex_decode(key_str)
	local osize = 42

	local expected = '3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865'
	local result = assert(crypto.hkdf('SHA256',salt,info,key,osize))
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)),expected)
end

function testCrypto:test_hkdf2()
	-- Test Case 2
	local salt_str = '606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf'
	local info_str = 'b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff'
	local key_str = '000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f'

	local salt = llae.buffer.hex_decode(salt_str)
	local info = llae.buffer.hex_decode(info_str)
	local key = llae.buffer.hex_decode(key_str)
	local osize = 82

	local expected = 'b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87'
	local result = assert(crypto.hkdf('SHA256',salt,info,key,osize))
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)),expected)
end

function testCrypto:test_hkdf3()
	-- Test Case 3 - with nil salt and info
	local key_str = '0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b'

	local salt = nil
	local info = nil
	local key = llae.buffer.hex_decode(key_str)
	local osize = 42

	local expected = '8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8'
	local result = assert(crypto.hkdf('SHA256',salt,info,key,osize))
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)),expected)
end

function testCrypto:test_hkdf_simple()
	-- Simple HKDF test with string inputs
	local key = 'secret_key'
	local salt = 'salt'
	local info = 'context_info'
	local osize = 32
	
	-- Should produce 32 bytes of output
	local result = assert(crypto.hkdf('SHA256', salt, info, key, osize))
	lu.assertEquals(#tostring(result), 32)
	
	-- Same inputs should produce same output (deterministic)
	local result2 = assert(crypto.hkdf('SHA256', salt, info, key, osize))
	lu.assertEquals(tostring(result), tostring(result2))
end

-- HMAC tests

function testCrypto:test_hmac_sha256()
	-- Test HMAC-SHA256
	-- Test vector verified with: echo -n "Hello, World!" | openssl dgst -sha256 -hmac "secret_key"
	local hmac = assert(crypto.hmac.new('SHA256'))
	assert(hmac:start('secret_key'))
	assert(hmac:update('Hello, World!'))
	local result = assert(hmac:finish())
	
	local expected = 'd0e72e3ebca850380c42bc96009638375860cb5c330048588d3298f02e740065'
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_hmac_sha256_incremental()
	-- Test HMAC with incremental updates
	local hmac = assert(crypto.hmac.new('SHA256'))
	assert(hmac:start('secret_key'))
	assert(hmac:update('Hello, '))
	assert(hmac:update('World!'))
	local result = assert(hmac:finish())
	
	-- Should match single update result
	local expected = 'd0e72e3ebca850380c42bc96009638375860cb5c330048588d3298f02e740065'
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_hmac_sha256_long_text()
	-- Test HMAC with longer text
	-- Test vector: echo -n "The quick brown fox jumps over the lazy dog" | openssl dgst -sha256 -hmac "key"
	local hmac = assert(crypto.hmac.new('SHA256'))
	assert(hmac:start('key'))
	assert(hmac:update('The quick brown fox jumps over the lazy dog'))
	local result = assert(hmac:finish())
	
	local expected = 'f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8'
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_hmac_sha256_empty()
	-- Test HMAC with empty data
	-- Test vector: echo -n "" | openssl dgst -sha256 -hmac "key"
	local hmac = assert(crypto.hmac.new('SHA256'))
	assert(hmac:start('key'))
	assert(hmac:update(''))
	local result = assert(hmac:finish())
	
	local expected = '5d5d139563c95b5967b9bd9a8c9b233a9dedb45072794cd232dc1b74832607d0'
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_hmac_md5()
	-- Test HMAC-MD5
	-- Test vector: echo -n "test" | openssl dgst -md5 -hmac "secret"
	local hmac = assert(crypto.hmac.new('MD5'))
	assert(hmac:start('secret'))
	assert(hmac:update('test'))
	local result = assert(hmac:finish())
	
	local expected = '63d6baf65df6bdee8f32b332e0930669'
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_hmac_reset()
	-- Test HMAC reset functionality
	local hmac = assert(crypto.hmac.new('SHA256'))
	
	-- First computation
	assert(hmac:start('key1'))
	assert(hmac:update('data1'))
	local result1 = assert(hmac:finish())
	
	-- Reset and compute with different key/data
	assert(hmac:reset())
	assert(hmac:start('key2'))
	assert(hmac:update('data2'))
	local result2 = assert(hmac:finish())
	
	-- Results should be different
	lu.assertNotEquals(tostring(result1), tostring(result2))
	
	-- Reset and compute again with first key/data
	assert(hmac:reset())
	assert(hmac:start('key1'))
	assert(hmac:update('data1'))
	local result3 = assert(hmac:finish())
	
	-- Should match first result
	lu.assertEquals(tostring(result1), tostring(result3))
end

-- Cipher tests

function testCrypto:test_aes_256_cbc_encrypt()
	-- Test AES-256-CBC encryption with PKCS7 padding
	-- Key: 603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4
	-- IV:  000102030405060708090a0b0c0d0e0f
	-- Plaintext: "Hello, World!"
	-- Expected ciphertext: 481290912e8cdf083258ddb453f24db8
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local plaintext = 'Hello, World!'
	local expected = '481290912e8cdf083258ddb453f24db8'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.ENCRYPT))
	assert(cipher:set_iv(iv))
	assert(cipher:set_padding(crypto.PADDING_PKCS7))

	local encrypted = assert(cipher:update(plaintext))
	local final = assert(cipher:finish())
	
	local result = encrypted .. final
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_aes_256_cbc_decrypt()
	-- Test AES-256-CBC decryption with PKCS7 padding
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local ciphertext = assert(llae.buffer.hex_decode('481290912e8cdf083258ddb453f24db8'))
	local expected = 'Hello, World!'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.DECRYPT))
	assert(cipher:set_iv(iv))
	assert(cipher:set_padding(crypto.PADDING_PKCS7))

	local decrypted = assert(cipher:update(ciphertext))
	local final = assert(cipher:finish())
	
	local result = decrypted .. final
	lu.assertEquals(tostring(result), expected)
end

function testCrypto:test_aes_256_cbc_encrypt_longer()
	-- Test AES-256-CBC encryption with longer text
	-- Plaintext: "The quick brown fox jumps over the lazy dog"
	-- Expected: 993f48c817946d0ccba1d7c53813cf8441e613d2cb47645dc1825884add9b9c9b0b2a3596ecd601df726bbaa5c087b72
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local plaintext = 'The quick brown fox jumps over the lazy dog'
	local expected = '993f48c817946d0ccba1d7c53813cf8441e613d2cb47645dc1825884add9b9c9b0b2a3596ecd601df726bbaa5c087b72'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.ENCRYPT))
	assert(cipher:set_iv(iv))
	assert(cipher:set_padding(crypto.PADDING_PKCS7))

	local encrypted = assert(cipher:update(plaintext))
	local final = assert(cipher:finish())
	
	local result = tostring(encrypted) .. tostring(final)
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_aes_256_cbc_crypt()
	-- Test one-shot crypt() method
	-- This should produce the same result as multi-step encryption
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local plaintext = 'Hello, World!'
	local expected = '481290912e8cdf083258ddb453f24db8'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.ENCRYPT))
	assert(cipher:set_padding(crypto.PADDING_PKCS7))

	local encrypted = assert(cipher:crypt(iv, plaintext))
	lu.assertEquals(tostring(llae.buffer.hex_encode(encrypted)), expected)
end

function testCrypto:test_cipher_padding_none()
	-- Test AES-256-CBC with no padding (input must be exact block size: 16 bytes)
	-- Plaintext: "1234567890123456" (exactly 16 bytes)
	-- Expected: 61fd495251f8f144ccbf89c8d6ec013b
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local plaintext = '1234567890123456'
	local expected = '61fd495251f8f144ccbf89c8d6ec013b'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.ENCRYPT))
	assert(cipher:set_iv(iv))
	assert(cipher:set_padding(crypto.PADDING_NONE))

	local encrypted = assert(cipher:update(plaintext))
	local final = assert(cipher:finish())

	local result = encrypted .. final
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)), expected)
end

function testCrypto:test_aes_256_gcm_auth_encrypt()
	-- Test AES-256-GCM authenticated encryption
	-- GCM is an AEAD (Authenticated Encryption with Associated Data) mode
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c'))
	local plaintext = 'Hello, World!'
	local ad = 'additional data'  -- Additional authenticated data
	local tag_len = 16

	local cipher = assert(crypto.cipher.new('AES-256-GCM'))
	assert(cipher:set_key(key, crypto.ENCRYPT))

	-- Encrypt with authentication
	local encrypted = assert(cipher:auth_encrypt(iv, ad, plaintext, tag_len))
	
	-- The result should include both ciphertext and authentication tag
	lu.assertNotNil(encrypted)
	lu.assertTrue(#tostring(encrypted) > #plaintext)
end

function testCrypto:test_aes_256_gcm_auth_decrypt()
	-- Test AES-256-GCM authenticated decryption (round-trip test)
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c'))
	local plaintext = 'Secret message!'
	local ad = 'metadata'
	local tag_len = 16

	-- First, encrypt
	local cipher_enc = assert(crypto.cipher.new('AES-256-GCM'))
	assert(cipher_enc:set_key(key, crypto.ENCRYPT))
	local encrypted = assert(cipher_enc:auth_encrypt(iv, ad, plaintext, tag_len))

	-- Then, decrypt
	local cipher_dec = assert(crypto.cipher.new('AES-256-GCM'))
	assert(cipher_dec:set_key(key, crypto.DECRYPT))
	local decrypted = assert(cipher_dec:auth_decrypt(iv, ad, encrypted, tag_len))

	-- Decrypted text should match original plaintext
	lu.assertEquals(tostring(decrypted), plaintext)
end

function testCrypto:test_cipher_reset()
	-- Test cipher reset functionality for reuse
	local key = assert(llae.buffer.hex_decode('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4'))
	local iv = assert(llae.buffer.hex_decode('000102030405060708090a0b0c0d0e0f'))
	local plaintext = 'Test message'

	local cipher = assert(crypto.cipher.new('AES-256-CBC'))
	assert(cipher:set_key(key, crypto.ENCRYPT))
	assert(cipher:set_padding(crypto.PADDING_PKCS7))

	-- First encryption
	local encrypted1 = assert(cipher:crypt(iv, plaintext))

	-- Reset and encrypt again with same parameters
	assert(cipher:reset())
	local encrypted2 = assert(cipher:crypt(iv, plaintext))

	-- Both encryptions should produce the same result
	lu.assertEquals(tostring(encrypted1), tostring(encrypted2))
end

-- ECP (Elliptic Curve) tests

function testCrypto:test_ecp_point_operations()
	-- Test basic ecp_point operations
	local point1 = assert(crypto.ecp_point.new())
	local point2 = assert(crypto.ecp_point.new())
	
	-- Test set_zero and is_zero
	point1:set_zero()
	lu.assertTrue(point1:is_zero())
	
	-- Test comparison
	point2:set_zero()
	lu.assertTrue(point1:cmp(point2))
	
	-- Different points should not be equal (after we set one to non-zero)
	-- This will be tested more thoroughly in serialization tests
end

function testCrypto:test_ecp_gen_keypair()
	-- Test generating a key pair on secp256r1 curve
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Generate key pair
	local privkey, pubkey = assert(ecp:gen_keypair())
	
	-- Both should be non-nil
	lu.assertNotNil(privkey)
	lu.assertNotNil(pubkey)
	
	-- Public key should not be zero point
	lu.assertFalse(pubkey:is_zero())
end

function testCrypto:test_ecp_gen_pubkey()
	-- Test deriving public key from private key
	-- Using known private key from openssl: 76bfcc6356ce1aa91c5bb0c98e5f0f6c542359f11ddfb9c7cb34b1e5f5959b32
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Create bignum with private key
	local privkey = assert(crypto.bignum.new())
	privkey:read(assert(llae.buffer.hex_decode('76bfcc6356ce1aa91c5bb0c98e5f0f6c542359f11ddfb9c7cb34b1e5f5959b32')))
	
	-- Generate public key from private key
	local pubkey = assert(ecp:gen_pubkey(privkey))
	
	-- Public key should not be zero
	lu.assertNotNil(pubkey)
	lu.assertFalse(pubkey:is_zero())
end

function testCrypto:test_ecp_check_keys()
	-- Test key validation
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Generate valid key pair
	local privkey, pubkey = assert(ecp:gen_keypair())
	
	-- Valid keys should pass validation
	lu.assertTrue(ecp:check_privkey(privkey))
	lu.assertTrue(ecp:check_pubkey(pubkey))
	
	-- Zero point should not be valid public key
	local zero_point = assert(crypto.ecp_point.new())
	zero_point:set_zero()
	lu.assertFalse(ecp:check_pubkey(zero_point))
end

function testCrypto:test_ecp_point_serialization()
	-- Test point binary serialization (write/read)
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Generate a key pair to get a valid point
	local privkey, pubkey = assert(ecp:gen_keypair())
	
	-- Test uncompressed format
	local binary_uncompressed = assert(ecp:point_write_binary(pubkey, crypto.ECP_PF_UNCOMPRESSED))
	lu.assertNotNil(binary_uncompressed)
	lu.assertTrue(#tostring(binary_uncompressed) > 0)
	
	-- Read back the point
	local pubkey_restored = assert(ecp:point_read_binary(binary_uncompressed))
	lu.assertNotNil(pubkey_restored)
	
	-- Restored point should equal original
	lu.assertTrue(pubkey:cmp(pubkey_restored))
	
	-- Test compressed format
	local binary_compressed = assert(ecp:point_write_binary(pubkey, crypto.ECP_PF_COMPRESSED))
	lu.assertNotNil(binary_compressed)
	
	-- Compressed should be smaller than uncompressed
	lu.assertTrue(#tostring(binary_compressed) < #tostring(binary_uncompressed))
	
	-- Read back compressed point
	local pubkey_compressed = assert(ecp:point_read_binary(binary_compressed))
	lu.assertTrue(pubkey:cmp(pubkey_compressed))
end

function testCrypto:test_ecp_point_known_pubkey()
	-- Test reading known public key from openssl
	-- Public key (uncompressed): 046ea77844f7809f449d1a5c0a47727e60f085d70c1806a0f0ee95b9b62b6adb7d12c1c3fae6b1b5453effec0956c999c30af40f0b7e88aac44cef56d62e6ad701
	local ecp = assert(crypto.ecp.new('secp256r1'))
	local pubkey_hex = '046ea77844f7809f449d1a5c0a47727e60f085d70c1806a0f0ee95b9b62b6adb7d12c1c3fae6b1b5453effec0956c999c30af40f0b7e88aac44cef56d62e6ad701'
	local pubkey_bin = assert(llae.buffer.hex_decode(pubkey_hex))
	
	-- Read the public key
	local pubkey = assert(ecp:point_read_binary(pubkey_bin))
	lu.assertNotNil(pubkey)
	lu.assertFalse(pubkey:is_zero())
	
	-- Should be valid public key
	lu.assertTrue(ecp:check_pubkey(pubkey))
	
	-- Write it back and compare
	local pubkey_written = assert(ecp:point_write_binary(pubkey, crypto.ECP_PF_UNCOMPRESSED))
	lu.assertEquals(tostring(llae.buffer.hex_encode(pubkey_written)), pubkey_hex)
end

function testCrypto:test_ecdsa_sign_verify_roundtrip()
	-- Test ECDSA sign and verify (round-trip)
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Generate key pair
	local privkey, pubkey = assert(ecp:gen_keypair())
	
	-- Message to sign
	local message = 'Hello, World!'
	local hash = assert(crypto.sha256sum(message))
	
	-- Sign the hash
	local r, s = assert(ecp:ecdsa_sign(hash, privkey))
	lu.assertNotNil(r)
	lu.assertNotNil(s)
	
	-- Verify the signature (order: hash, pubkey, r, s)
	local valid = assert(ecp:ecdsa_verify(hash, pubkey, r, s))
	lu.assertTrue(valid)
end

function testCrypto:test_ecdsa_verify_known_signature()
	-- Test ECDSA verification with known signature from openssl
	-- Note: This test uses non-deterministic signature, so we can't verify exact r,s values
	-- Instead we test the round-trip: we know the private key, derive pubkey, and verify
	-- Private key: 76bfcc6356ce1aa91c5bb0c98e5f0f6c542359f11ddfb9c7cb34b1e5f5959b32
	-- Public key: 046ea77844f7809f449d1a5c0a47727e60f085d70c1806a0f0ee95b9b62b6adb7d12c1c3fae6b1b5453effec0956c999c30af40f0b7e88aac44cef56d62e6ad701
	
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Load private key
	local privkey = assert(crypto.bignum.new())
	privkey:read(assert(llae.buffer.hex_decode('76bfcc6356ce1aa91c5bb0c98e5f0f6c542359f11ddfb9c7cb34b1e5f5959b32')))
	
	-- Derive public key from private key
	local pubkey = assert(ecp:gen_pubkey(privkey))
	
	-- Verify the derived public key matches expected
	local pubkey_written = assert(ecp:point_write_binary(pubkey, crypto.ECP_PF_UNCOMPRESSED))
	local expected_pubkey = '046ea77844f7809f449d1a5c0a47727e60f085d70c1806a0f0ee95b9b62b6adb7d12c1c3fae6b1b5453effec0956c999c30af40f0b7e88aac44cef56d62e6ad701'
	lu.assertEquals(tostring(llae.buffer.hex_encode(pubkey_written)), expected_pubkey)
	
	-- Now sign and verify
	local message = 'Hello, World!'
	local hash = assert(crypto.sha256sum(message))
	
	-- Sign
	local r, s = assert(ecp:ecdsa_sign(hash, privkey))
	
	-- Verify with the public key
	local valid = assert(ecp:ecdsa_verify(hash, pubkey, r, s))
	lu.assertTrue(valid)
end

function testCrypto:test_ecdsa_verify_fails_wrong_hash()
	-- Test that verification fails with wrong hash
	local ecp = assert(crypto.ecp.new('secp256r1'))
	
	-- Generate key pair
	local privkey, pubkey = assert(ecp:gen_keypair())
	
	-- Sign one message
	local hash1 = assert(crypto.sha256sum('Message 1'))
	local r, s = assert(ecp:ecdsa_sign(hash1, privkey))
	
	-- Try to verify with different message
	local hash2 = assert(crypto.sha256sum('Message 2'))
	local valid, err = ecp:ecdsa_verify(hash2, pubkey, r, s)
	
	-- Should fail verification
	lu.assertFalse(valid == true)
end

function testCrypto:test_ecdh()
	-- Test ECDH (Elliptic Curve Diffie-Hellman)
	local ecp1 = assert(crypto.ecp.new('secp256r1'))
	local ecp2 = assert(crypto.ecp.new('secp256r1'))
	
	-- Party 1 generates key pair
	local priv1, pub1 = assert(ecp1:ecdh_gen_public())
	lu.assertNotNil(priv1)
	lu.assertNotNil(pub1)
	
	-- Party 2 generates key pair
	local priv2, pub2 = assert(ecp2:ecdh_gen_public())
	lu.assertNotNil(priv2)
	lu.assertNotNil(pub2)
	
	-- Party 1 computes shared secret using their private key and party 2's public key
	local shared1 = assert(ecp1:ecdh_compute_shared(pub2, priv1))
	
	-- Party 2 computes shared secret using their private key and party 1's public key
	local shared2 = assert(ecp2:ecdh_compute_shared(pub1, priv2))
	
	-- Both parties should compute the same shared secret
	local shared1_hex = tostring(llae.buffer.hex_encode(assert(shared1:write())))
	local shared2_hex = tostring(llae.buffer.hex_encode(assert(shared2:write())))
	lu.assertEquals(shared1_hex, shared2_hex)
end

-- PK (Public Key / RSA) tests

function testCrypto:test_pk_parse_public_key()
	-- Test parsing RSA public key from PEM format
	-- Create pk instance
	local pk = assert(crypto.pk.new())
	
	-- Parse public key
	assert(pk:parse_public_key(rsa_public_key_pem))
	
	-- If parsing succeeded, we should be able to get key name
	local name = assert(pk:get_name())
	lu.assertEquals(name, "RSA")
end

function testCrypto:test_pk_get_name()
	-- Test getting key type name
	local pk = assert(crypto.pk.new())
	assert(pk:parse_public_key(rsa_public_key_pem))
	
	-- Get key name
	local name = assert(pk:get_name())
	lu.assertEquals(name, "RSA")
end

function testCrypto:test_pk_get_rsa()
	-- Test getting RSA context from pk instance
	local pk = assert(crypto.pk.new())
	assert(pk:parse_public_key(rsa_public_key_pem))
	
	-- Get RSA context
	local rsa = assert(pk:get_rsa())
	lu.assertNotNil(rsa)
end

function testCrypto:test_pk_rsa_padding()
	-- Test setting RSA padding modes
	local pk = assert(crypto.pk.new())
	assert(pk:parse_public_key(rsa_public_key_pem))
	local rsa = assert(pk:get_rsa())
	
	-- Test RSA_PKCS_V15 padding
	assert(rsa:set_padding(crypto.RSA_PKCS_V15))
	
	-- Test RSA_PKCS_V21 (OAEP) padding
	assert(rsa:set_padding(crypto.RSA_PKCS_V21))
end

function testCrypto:test_pk_encrypt()
	-- Test RSA encryption
	local pk = assert(crypto.pk.new())
	assert(pk:parse_public_key(rsa_public_key_pem))
	
	-- Encrypt some data
	local plaintext = "Hello, RSA!"
	local encrypted = assert(pk:encrypt(plaintext))
	
	-- Encrypted data should be different from plaintext
	lu.assertNotEquals(tostring(encrypted), plaintext)
	
	-- Encrypted data should be non-empty
	lu.assertTrue(#tostring(encrypted) > 0)
	
	-- For RSA 2048, encrypted output should be 256 bytes
	lu.assertEquals(#tostring(encrypted), 256)
end

function testCrypto:test_pk_parse_invalid()
	-- Test parsing invalid key data
	local pk = assert(crypto.pk.new())
	
	-- Try to parse invalid data
	local invalid_data = "This is not a valid PEM key"
	local success, err = pk:parse_public_key(invalid_data)
	
	-- Should fail
	lu.assertNil(success)
	lu.assertNotNil(err)
end


function testCrypto:test_stress()
	local data = 'test'
	local t = os.time()
	while os.difftime(os.time(),t) < 2 do
		local md = assert(crypto.md.new('MD5'))
		assert(md:update(data))
		data = assert(md:finish())
	end
end