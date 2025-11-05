local lu = require 'luaunit'

local bind_tests = require 'bind_tests'


TestBindings = {}


function TestBindings:test_bindings()
	local b1 = bind_tests.test_bind_fields.new()
    b1:method1()
    b1.field1 = 1
    b1.field2 = 2.0
    b1.field3 = "3"
    lu.assertEquals(b1.field1, 1)
    lu.assertEquals(b1.field2, 2.0)
    lu.assertEquals(b1.field3, "3")

    local b2 = bind_tests.test_bind_fields.new()
    b2.field1 = 4
    b2.field2 = 5.0
    b2.field3 = "6"
    lu.assertEquals(b2.field1, 4)
    lu.assertEquals(b2.field2, 5.0)
    lu.assertEquals(b2.field3, "6")
end

function TestBindings:test_get_const()
    local b = bind_tests.test_bind_fields.new()
    b.field1 = 1
    local c = b:get_const()
    lu.assertEquals(c.field1, 1)
    lu.assertErrorMsgEquals(
        "invalid self object tests::test_bind_fields is const",
        function() 
            c.field2 = 2.0 
        end
    )
    lu.assertEquals(c.const_field, 5)
    lu.assertEquals(b.const_field, 5)
    lu.assertErrorMsgContains(
        "set read only field const_field at tests::test_bind_fields",
        function() 
            b.const_field = 6
        end
    )
end

function TestBindings:test_return_ref()
    do 
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
        local b = bind_tests.test_bind_fields.new()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        b = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
    end
    do
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
        local b = bind_tests.test_bind_fields.new()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        local c = b:get_self()
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        b = nil
        collectgarbage()
        c.field1 = 2
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        c.field1 = 1
        c = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
    end
end