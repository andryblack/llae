local lu = require 'luaunit'
local llae = require 'llae'
local crypto = require 'crypto'

local coro_tests = require 'coro_tests'


TestCoro = {}


function TestCoro:test_md()
    local r = coro_tests.test_md()
    lu.assertNotIsNil(r)
    local res,err = r:await()
    lu.assertNotIsNil(res)
    lu.assertIsNil(err)
    lu.assertEquals(tostring(llae.buffer.hex_encode(res)),'098f6bcd4621d373cade4e832627b4f6')
end

function TestCoro:test_async()
    local m = crypto.md.new('MD5')
    assert(m:update('test'))
    local r = assert(m:finish())
    lu.assertEquals(tostring(llae.buffer.hex_encode(r)),'098f6bcd4621d373cade4e832627b4f6')
end