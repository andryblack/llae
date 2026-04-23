local lu = require 'luaunit'

local bind_tests = require 'autobind_tests'


TestAutoBindings = {}


function TestAutoBindings:test_bindings()
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

function TestAutoBindings:test_get_const()
    local b = bind_tests.test_bind_fields.new()
    b.field1 = 1
    local c = b:get_const()
    lu.assertEquals(c.field1, 1)
    lu.assertErrorMsgEquals(
        "invalid self object autobind_tests::test_bind_fields is const",
        function() 
            c.field2 = 2.0 
        end
    )
    lu.assertEquals(c.const_field, 5)
    lu.assertEquals(b.const_field, 5)
    lu.assertErrorMsgContains(
        "set read only field const_field at autobind_tests::test_bind_fields",
        function() 
            b.const_field = 6
        end
    )
end

function TestAutoBindings:test_return_ref()
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

    do 
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
        local b = bind_tests.test_bind_fields.new()
        b.field4.x = 100500
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        local f = b:get_const_field4()
        lu.assertEquals(f.x, 100500)
        lu.assertErrorMsgContains(
            "invalid self object autobind_tests::test_bind_struct is const",
            function() 
                f.x = 300
            end
        )
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        b = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        f = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
    end

    do 
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
        local b = bind_tests.test_bind_fields.new()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        local f = b:get_field4()
        f.x = 100500
        lu.assertEquals(b.field4.x, 100500)
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        b = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
        f = nil
        collectgarbage()
        lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
    end
end

function TestAutoBindings:test_optional_ref()
    local b = bind_tests.test_bind_fields.new()
    lu.assertIsNil(b.optional_field5)
    b.optional_field5 = b.field4
    lu.assertNotIsNil(b.optional_field5)
    b.optional_field5.x = 100500
    lu.assertEquals(b.optional_field5.x, 100500)
    local f = b:get_const_optional_field5()
    lu.assertEquals(f.x, 100500)
end

function TestAutoBindings:test_array_ref()
    local b = bind_tests.test_bind_fields.new()
    b.array1[1] = 1
    b.array1[2] = 2
    b.array1[3] = 3
    b.array1[4] = 4
    b.array1[5] = 5
    lu.assertEquals(b.array1[1], 1)
    lu.assertEquals(b.array1[2], 2)
    lu.assertEquals(b.array1[3], 3)
    lu.assertEquals(b.array1[4], 4)
    lu.assertEquals(b.array1[5], 5)

    local c = b:get_const()

    lu.assertEquals(c.array1[4], 4)

    lu.assertErrorMsgContains(
        "attempt to modify a const array",
        function() 
            c.array1[1] = 1
        end
    )
    

end

function TestAutoBindings:test_array_ref_ref()
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
    local b = bind_tests.test_bind_fields.new()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    local a = b.array1
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    b = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    a[4] = 5
    a = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
end

function TestAutoBindings:test_field_ref()
    local b = bind_tests.test_bind_fields.new()
    b.field4.x = 1
    lu.assertEquals(b.field4.x, 1)
    lu.assertEquals(b.field4.y, 0)
    local f = b.field4
    b = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    f = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
end

function TestAutoBindings:test_field_ref_const()
    local b = bind_tests.test_bind_fields.new()
    b.field4.x = 1
    lu.assertEquals(b.field4.x, 1)
    lu.assertEquals(b.field4.y, 0)
    local c = b:get_const()
    lu.assertEquals(c.field4.x, 1)
    lu.assertEquals(c.field4.y, 0)
    local f = c.field4
    lu.assertEquals(f.x, 1)
    lu.assertEquals(f.y, 0)
    lu.assertErrorMsgContains(
        "invalid self object autobind_tests::test_bind_struct is const",
        function() 
            f.x = 2
        end
    )
    b.field4 = f
    lu.assertErrorMsgContains(
        "invalid reference: need autobind_tests::test_bind_struct, got table",
        function() 
            b.field4 = {}
        end
    )
end

function TestAutoBindings:test_field_ref_array()
    local b = bind_tests.test_bind_fields.new()
    b.array2[1].x = 1
    b.array2[2].y = 2
    lu.assertEquals(b.array2[1].x, 1)
    lu.assertEquals(b.array2[2].y, 2)
    local a = b.array2
    b = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    local v = a[1]
    a = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 1)
    v = nil
    collectgarbage()
    lu.assertEquals(bind_tests.test_bind_fields.get_count(), 0)
end

function TestAutoBindings:test_field_ref_array_const()
    local b = bind_tests.test_bind_fields.new()
    b.array2[1].x = 1
    b.array2[2].y = 2
    lu.assertEquals(b.array2[1].x, 1)
    lu.assertEquals(b.array2[2].y, 2)
    local c = b:get_const()
    lu.assertEquals(c.array2[1].x, 1)
    lu.assertEquals(c.array2[2].y, 2)
    lu.assertErrorMsgContains(
        "invalid self object autobind_tests::test_bind_struct is const",
        function() 
            c.array2[1].x = 2
        end
    )
    lu.assertErrorMsgContains(
        "invalid reference: need autobind_tests::test_bind_struct, got nil",
        function() 
            b.array2[1] = nil
        end
    )
    lu.assertErrorMsgContains(
        "attempt to set array field",
        function() 
            b.array2 = {}
        end
    )
end

function TestAutoBindings:test_array_bounds()
    local b = bind_tests.test_bind_fields.new()
    lu.assertErrorMsgContains(
        "index out of bounds",
        function() 
            b.array1[0] = 1
        end
    )
    lu.assertErrorMsgContains(
        "index out of bounds",
        function() 
            b.array1[6] = 1
        end
    )
    b.array1[1] = 1
    b.array1[5] = 5

    lu.assertIsNil(b.array1[0])
    lu.assertIsNil(b.array1[6])
    lu.assertEquals(#b.array1, 5)
    lu.assertErrorMsgContains(
        [[bad argument #2 to '__newindex' (number expected, got string)]],
        function() 
            b.array1['x'] = 1
        end
    )
end

function TestAutoBindings:test_array_iterator()
    local b = bind_tests.test_bind_fields.new()
    for i=1,5 do
        b.array1[i] = i*2
        b.array2[i].x = i*3
        b.array2[i].y = i*4
    end
    for i,v in ipairs(b.array1) do
        lu.assertEquals(v, i*2)
    end
    for i,v in ipairs(b.array2) do
        lu.assertEquals(v.x, i*3)
        lu.assertEquals(v.y, i*4)
    end

    --local a,b = next(b.array1)
end

function TestAutoBindings:test_string_field()
    local b = bind_tests.test_bind_fields.new()
    b.string_field = "hello"
    lu.assertEquals(b.string_field, "hello")
    b.string_field = "hello worl"
    lu.assertEquals(b.string_field, "hello worl")
    lu.assertErrorMsgContains(
        "string too long",
        function() 
            b.string_field = "hello world hello world"
        end
    )
    local d = b.data_field
    lu.assertEquals(#d, 10)
    b.data_field = "hello\0worl"
    lu.assertEquals(b.data_field, "hello\0worl")
end

function TestAutoBindings:test_value()
    lu.assertEquals(bind_tests.test_value, 123)
end