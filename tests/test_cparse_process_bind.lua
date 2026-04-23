local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local process_bind = require('cparse.process_bind')
package.path = store_path

TestProcessBind = {}


function TestProcessBind:test_process_bind()
    local processor = process_bind.new()
    local result = processor:process_content[[

namespace foo {
    /// @luabind
    class bar {
        /// @luabind
        class baz {
            /// @luabind
            int x;
            /// @luabind
            void method1();
        };
        /// @luabind
        void method2();
    };
}

]]
    lu.assertIsTable(result)
    lu.assertIsTable(result.classes)
    lu.assertEquals(#result.classes, 2)
    local cls1 = result.classes[1]
    lu.assertEquals(cls1:get_name(), 'bar')
    lu.assertEquals(cls1:get_prefix(), 'foo')
    local cls2 = result.classes[2]
    lu.assertEquals(cls2:get_name(), 'baz')
    lu.assertEquals(cls2:get_prefix(), 'foo::bar')
end

function TestProcessBind:test_process_bind_with_extern_doc_block()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace extern_bind {
    /** @extern
    enum class test_module_enum { a, b };
    struct test_bind_struct {
        int x = 0;
    };
    static void test_function1(int) {}
    constexpr int test_value = 123;
    */
}
]]
    lu.assertIsTable(result)
    lu.assertIsTable(result.modules)
    lu.assertNotNil(result.modules['extern_bind'])
    local module = result.modules['extern_bind']
    lu.assertEquals(#module:get_classes(), 1)
    lu.assertEquals(module:get_classes()[1]:get_name(), 'test_bind_struct')
    lu.assertEquals(#module:get_enums(), 1)
    lu.assertEquals(module:get_enums()[1]:get_name(), 'test_module_enum')
    lu.assertEquals(#module:get_functions(), 1)
    lu.assertEquals(module:get_functions()[1]:get_name(), 'test_function1')
    lu.assertEquals(#module:get_values(), 1)
    lu.assertEquals(module:get_values()[1]:get_name(), 'test_value')
end

function TestProcessBind:test_process_bind_extern_field_directive()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace fb {
    /** @extern
    struct S {
        @field(x, readonly=true) integer field from extern docs
    };
    */
}
]]
    lu.assertNotNil(result.modules['fb'])
    local module = result.modules['fb']
    lu.assertEquals(#module:get_classes(), 1)
    local cls = module:get_classes()[1]
    lu.assertEquals(cls:get_name(), 'S')
    local fields = cls:get_fields()
    lu.assertEquals(#fields, 1)
    lu.assertEquals(fields[1]:get_name(), 'x')
    lu.assertEquals(fields[1]:get_type(), 'unknown_binding_type')
    lu.assertEquals(fields[1]:get_bind('readonly'), 'true')
    lu.assertStrContains(fields[1]:get_lua_comments(), 'integer field from extern docs')
end

function TestProcessBind:test_process_bind_extern_field_and_func_at_namespace()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace ns_ex {
    /** @extern
    @field(K, foo=1)
    @func(f, bar=2)
    */
}
]]
    lu.assertNotNil(result.modules['ns_ex'])
    local module = result.modules['ns_ex']
    lu.assertEquals(#module:get_values(), 1)
    lu.assertEquals(module:get_values()[1]:get_name(), 'K')
    lu.assertEquals(module:get_values()[1]:get_type(), 'extern_constant')
    lu.assertEquals(module:get_values()[1]:get_bind('foo'), '1')
    lu.assertEquals(#module:get_functions(), 1)
    lu.assertEquals(module:get_functions()[1]:get_name(), 'f')
    lu.assertEquals(module:get_functions()[1]:get_bind('bar'), '2')
end


function TestProcessBind:test_process_bind_extern_func_tags()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace ns_ex {
    /** @extern
    /// @lparam(x,integer)
    /// @lreturn(res,string?)
    @func(f, bar=2)
    struct S {
        @field(x,type=integer) is a x field
        /// this is g function
        /// @lparam(x,integer)
        /// @lreturn(res,string?)
        @func(g)
    };
    */
}
]]
    lu.assertNotNil(result.modules['ns_ex'])
    local module = result.modules['ns_ex']
    lu.assertEquals(#module:get_functions(), 1)
    local func = module:get_functions()[1]
    lu.assertEquals(func:get_name(), 'f')
    lu.assertEquals(#func:get_lua_args(), 1)
    local arg_x = func:get_lua_args()[1]
    lu.assertEquals(arg_x.name, 'x')
    lu.assertEquals(arg_x.type, 'integer')
    lu.assertEquals(#func:get_lua_results(), 1)
    local result_res = func:get_lua_results()[1]
    lu.assertEquals(result_res.name, 'res')
    lu.assertEquals(result_res.type, 'string?')
    lu.assertEquals(#module:get_classes(), 1)
    local cls = module:get_classes()[1]
    lu.assertEquals(cls:get_name(), 'S')
    lu.assertEquals(#cls:get_methods(), 1)
    local func_g = cls:get_methods()[1]
    lu.assertEquals(func_g:get_name(), 'g')
    lu.assertEquals(#func_g:get_lua_args(), 1)
    local arg_x_g = func_g:get_lua_args()[1]
    lu.assertEquals(arg_x_g.name, 'x')
    lu.assertEquals(arg_x_g.type, 'integer')
    lu.assertEquals(#func_g:get_lua_results(), 1)
    local result_res_g = func_g:get_lua_results()[1]
    lu.assertEquals(result_res_g.name, 'res')
    lu.assertEquals(result_res_g.type, 'string?')
    lu.assertStrContains(func_g:get_lua_comments(), 'this is g function')
    lu.assertEquals(#cls:get_fields(), 1)
    local field_x = cls:get_fields()[1]
    lu.assertEquals(field_x:get_name(), 'x')
    lu.assertEquals(field_x:get_type(), 'integer')
    lu.assertStrContains(field_x:get_lua_comments(), 'is a x field')
end

