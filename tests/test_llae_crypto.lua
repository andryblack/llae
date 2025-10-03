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