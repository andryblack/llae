local fs = require 'llae.fs'
local lu = require 'luaunit'
local llae = require 'llae'
local crypto = require 'llae.crypto'

testCrypto = {}

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
	-- Test Case 3
	local key_str = '0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b'

	local salt = nil
	local info = nil
	local key = llae.buffer.hex_decode(key_str)
	local osize = 42

	local expected = '8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8'
	local result = assert(crypto.hkdf('SHA256',salt,info,key,osize))
	lu.assertEquals(tostring(llae.buffer.hex_encode(result)),expected)
end