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

