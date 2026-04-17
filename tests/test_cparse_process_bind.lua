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

