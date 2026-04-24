local lu = require('luaunit')
local template = require('llae.template')
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

function TestProcessBind:test_process_bind_with_luabind_parse_block()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace extern_bind {
    #ifdef LUABIND_PARSE
    /// @luabind
    enum class test_module_enum { a, b };
    /// @luabind
    struct test_bind_struct {
        /// @luabind
        int x = 0;
    };
    /// @luabind
    static void test_function1(int) {}
    /// @luabind
    constexpr int test_value = 123;
    #endif
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

function TestProcessBind:test_process_bind_with_external_stub_decls()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace fb {
    #ifdef LUABIND_PARSE
    /// @luabind
    struct S {
        /// @luabind(readonly=true)
        inline constexpr ::luabind_autobind_type x = {};
        /// @luabind
        ::luabind_autobind_type g();
    };
    #endif
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
    lu.assertEquals(fields[1]:get_type(), '::luabind_autobind_type')
    lu.assertEquals(fields[1]:get_bind('readonly'), 'true')
    lu.assertEquals(#cls:get_methods(), 1)
    local func_g = cls:get_methods()[1]
    lu.assertEquals(func_g:get_name(), 'g')
end

function TestProcessBind:test_meta_template_generates_enum_annotations()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace enum_meta {
    /// @luabind
    enum class module_enum { first = 1, second };

    /// @luabind
    class holder {
        /// @luabind
        enum class class_enum { alpha = 3, beta };

        /// @luabind
        void set_mode(module_enum mode);
        /// @luabind
        /// @lparam(x,integer)
        /// @lreturn(r,string)
        /// @loverload(fun(a:string,b:integet):boolean)
        void method();
    };
}
]]
    lu.assertNotNil(result.modules['enum_meta'])
    local module = result.modules['enum_meta']
    lu.assertEquals(module:resolve_lua_type('module_enum'), 'enum_meta.module_enum')

    local cls = module:get_classes()[1]
    lu.assertEquals(#cls:get_methods(), 2)
    local method = cls:get_methods()[1]
    lu.assertEquals(method:get_lua_args()[1].type, 'enum_meta.module_enum')

    local meta = template.render_file('data/binding-meta-template.lua', {
        escape = tostring,
        module = module,
    })
    local function normalize_text(text)
        text = text:gsub('\r\n', '\n')
        text = text:gsub('[ \t]+\n', '\n')
        text = text:gsub('\n\n', '\n')
        text = text:gsub('\n\n', '\n')
        text = text:gsub('%s+$', '')
        return text
    end
    meta = normalize_text(meta)
    local expected_meta = ([[
---@meta enum_meta
---@class enum_meta
local enum_meta = {}
--- module_enum
---@enum enum_meta.module_enum
local enum_meta_module_enum = {
  first = 1,
  second = 2,
}
enum_meta.module_enum = enum_meta_module_enum
--- holder
---@class enum_meta.holder
local holder = {}
--- class_enum
---@enum enum_meta.holder.class_enum
local enum_meta_holder_class_enum = {
  alpha = 3,
  beta = 4,
}
holder.class_enum = enum_meta_holder_class_enum
--- set_mode
---@param mode enum_meta.module_enum
function holder:set_mode(mode) end
--- method
---@param x integer
---@return string r
---@overload fun(a:string,b:integet):boolean
function holder:method(x) end
enum_meta.holder = holder
return enum_meta
    ]])
    expected_meta = normalize_text(expected_meta)
    lu.assertEquals(meta, expected_meta)
end

function TestProcessBind:test_process_bind_class_scope_all_true()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace scoped_all {
    /// @luabind(all=true)
    class Foo {
        void bar();
        int value = 0;
        enum class Kind { one = 1, two };
    };
}
]]
    lu.assertNotNil(result.modules['scoped_all'])
    local module = result.modules['scoped_all']
    lu.assertEquals(#module:get_classes(), 1)
    local cls = module:get_classes()[1]
    lu.assertEquals(cls:get_name(), 'Foo')
    lu.assertEquals(#cls:get_methods(), 1)
    lu.assertEquals(cls:get_methods()[1]:get_name(), 'bar')
    lu.assertEquals(#cls:get_fields(), 1)
    lu.assertEquals(cls:get_fields()[1]:get_name(), 'value')
    lu.assertEquals(#cls:get_enums(), 1)
    lu.assertEquals(cls:get_enums()[1]:get_name(), 'Kind')
end

function TestProcessBind:test_process_bind_namespace_scope_all_true()
    local processor = process_bind.new()
    local result = processor:process_content[[
/// @luabind(all=true)
namespace ns_all {
    class Foo {
        void bar();
    };
    static void ping(int);
    constexpr int answer = 42;
    enum class State { idle, active };
}
]]
    lu.assertNotNil(result.modules['ns_all'])
    local module = result.modules['ns_all']
    lu.assertEquals(#module:get_classes(), 1)
    local cls = module:get_classes()[1]
    lu.assertEquals(cls:get_name(), 'Foo')
    lu.assertEquals(#cls:get_methods(), 1)
    lu.assertEquals(cls:get_methods()[1]:get_name(), 'bar')
    lu.assertEquals(#module:get_functions(), 1)
    lu.assertEquals(module:get_functions()[1]:get_name(), 'ping')
    lu.assertEquals(#module:get_values(), 1)
    lu.assertEquals(module:get_values()[1]:get_name(), 'answer')
    lu.assertEquals(#module:get_enums(), 1)
    lu.assertEquals(module:get_enums()[1]:get_name(), 'State')
end

function TestProcessBind:test_process_bind_class_scope_all_true_extern_parse()
    local processor = process_bind.new()
    local result = processor:process_content[[
namespace ns_all_extern {
    #ifdef LUABIND_PARSE
    /// @luabind(all=true)
    struct S {
        ::luabind_autobind_type g();
    };
    #endif
}
]]
    lu.assertNotNil(result.modules['ns_all_extern'])
    local module = result.modules['ns_all_extern']
    lu.assertEquals(#module:get_classes(), 1)
    local cls = module:get_classes()[1]
    lu.assertEquals(cls:get_name(), 'S')
    lu.assertEquals(#cls:get_methods(), 1)
    lu.assertEquals(cls:get_methods()[1]:get_name(), 'g')
end

function TestProcessBind:test_process_bind_extern_fields()
    local processor = process_bind.new()
    local result = processor:process_content[[

/// @luabind(all=true)
namespace ns1::ns2 {

#ifdef LUABIND_PARSE

/// @luabind
struct Foo {
};

/// @luabind(type=ns1.ns2.Foo)
LUABIND_FIELD(bar)
/// @luabind(type=ns1.ns2.Foo)
LUABIND_FIELD(baz)
#endif
}
]]
    lu.assertNotNil(result.modules['ns1.ns2'])
    local module = result.modules['ns1.ns2']
    lu.assertEquals(#module:get_classes(), 1)
    lu.assertEquals(#module:get_values(), 2)
    lu.assertEquals(module:get_values()[1]:get_name(), 'bar')
    lu.assertEquals(module:get_values()[2]:get_name(), 'baz')
end