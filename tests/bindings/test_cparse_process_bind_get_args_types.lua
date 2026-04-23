local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local process_bind = require('cparse.process_bind')
package.path = store_path

TestProcessBindGetArgsTypes = {}

local function find_class(classes, name)
  for _, cls in ipairs(classes) do
    if cls:get_name() == name then
      return cls
    end
  end
  return nil
end


function TestProcessBindGetArgsTypes:test_constructor_args_use_module_namespace()
  local processor = process_bind.new()
  local result = processor:process_content[[
namespace arg_types {
  /// @luabind
  struct dep {};

  /// @luabind
  struct holder {
    /// @luabind(raw=true)
    holder(dep value, std::optional<dep> opt, dep* ptr, dep& ref);
  };
}
]]
  lu.assertNotNil(result.modules['arg_types'])
  local module = result.modules['arg_types']
  local holder = find_class(module:get_classes(), 'holder')
  lu.assertNotNil(holder)
  lu.assertNotNil(holder:get_constructor())

  local args = holder:get_constructor():get_args_types()
  lu.assertEquals(#args, 4)
  lu.assertEquals(args[1], 'arg_types::dep')
  lu.assertEquals(args[2], 'std::optional<arg_types::dep>')
  lu.assertEquals(args[3], 'arg_types::dep*')
  lu.assertEquals(args[4], 'arg_types::dep&')
end

function TestProcessBindGetArgsTypes:test_constructor_args_resolve_using_aliases()
  local processor = process_bind.new()
  local result = processor:process_content[[
namespace arg_types_alias {
  /// @luabind
  struct dep {};

  /// @luabind
  using dep_alias = dep;

  /// @luabind
  struct holder {
    /// @luabind(raw=true)
    holder(dep_alias value, dep_alias copy);
  };
}
]]
  lu.assertNotNil(result.modules['arg_types_alias'])
  local module = result.modules['arg_types_alias']
  local holder = find_class(module:get_classes(), 'holder')
  lu.assertNotNil(holder)
  lu.assertNotNil(holder:get_constructor())

  local args = holder:get_constructor():get_args_types()
  lu.assertEquals(#args, 2)
  lu.assertEquals(args[1], 'arg_types_alias::dep')
  lu.assertEquals(args[2], 'arg_types_alias::dep')
end

function TestProcessBindGetArgsTypes:test_inner_enum()
  local processor = process_bind.new()
  local result = processor:process_content[[
namespace module_name {
  /// @luabind
  struct class_name {
    /// @luabind
    enum class inner_enum { a, b, c };
    /// @luabind
    void set_inner_enum(inner_enum value);
  };
}
]]
  lu.assertNotNil(result.modules['module_name'])
  local module = result.modules['module_name']
  local holder = find_class(module:get_classes(), 'class_name')
  lu.assertNotNil(holder)
  lu.assertEquals(#holder:get_methods(), 1)
  local method = holder:get_methods()[1]
  lu.assertEquals(method:get_name(), 'set_inner_enum')
  local args = method:get_args_types()
  lu.assertEquals(#args, 1)
  lu.assertEquals(args[1], 'module_name::inner_enum')

end
