local lu = require 'luaunit'

TestCommon = {}


function TestCommon:test_inplace_function()
    local t = require 'inplace_function_tests'
    t.test()
end