local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local process_bind = require('cparse.process_bind')
package.path = store_path

TestProcessBindExternalMode = {}

function TestProcessBindExternalMode:test_external_declarations_are_parsed_via_process_content()
  local processor = process_bind.new()
  local result = processor:process_content([[
namespace ext {
  #ifdef LUABIND_PARSE
  /// @luabind
  enum class test_module_enum { a, b, c };
  /// @luabind
  struct test_bind_struct {
    /// @luabind
    int x = 0;
    /// @luabind
    enum class state {
      off,
      on,
    };
    /// @luabind
    state s;
  };
  #endif
}
  ]])

  lu.assertNotNil(result.modules['ext'])
  local module = result.modules['ext']
  lu.assertEquals(#module:get_enums(), 1)
  lu.assertEquals(module:get_enums()[1]:get_name(), 'test_module_enum')
  lu.assertEquals(#module:get_classes(), 1)
  local cls = module:get_classes()[1]
  lu.assertEquals(cls:get_name(), 'test_bind_struct')
  lu.assertEquals(#cls:get_fields(), 2)
  lu.assertEquals(cls:get_fields()[1]:get_name(), 'x')
  lu.assertEquals(cls:get_fields()[2]:get_name(), 's')
end

