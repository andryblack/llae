local lu = require 'luaunit'
local llae = require 'llae'
local crypto = require 'crypto'

local result_tests = require 'result_tests'


TestResult = {}


function TestResult:test_err()
    local r,e = result_tests.test_err()
    lu.assertIsNil(r)
    lu.assertNotIsNil(e)
    lu.assertEquals(tostring(e),'[llae]:failed')
end

function TestResult:test_assert()
    lu.assertErrorMsgEquals('tests/test_result.lua:20: failed',function()
        assert(nil,'failed')
    end)
    lu.assertErrorMsgEquals('tests/test_result.lua:23: [llae]:failed',function()
        assert(result_tests.test_err())
    end)
end

function TestResult:test_1()
   -- assert(result_tests.test_err())
end