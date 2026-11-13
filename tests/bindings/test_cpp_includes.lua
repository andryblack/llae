local lu = require('luaunit')
local fs = require('llae.fs')
local path = require('llae.path')
local log = require('llae.log')

local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local process_bind = require('cparse.process_bind')
package.path = store_path

local template = require('llae.template')
local utils = require('llae.utils')

TestCppIncludes = {}

local function write_file(filepath, content)
  fs.mkdir_r(path.dirname(filepath))
  fs.write_file(filepath, content)
end

function TestCppIncludes:setUp()
  self.tmp = path.join(fs.pwd(), 'build', 'tests', 'cpp_includes_tmp')
  if fs.isdir(self.tmp) then
    fs.rmdir_r(self.tmp)
  end
  fs.mkdir_r(self.tmp)
  self.root = path.join(self.tmp, 'project')
  self.src = path.join(self.root, 'src')
  self.bind_src = path.join(self.root, 'build', 'src')
  write_file(path.join(self.src, 'mylib/real_api.h'), '')
  write_file(path.join(self.src, 'mylib/api_extra.h'), '')
  write_file(path.join(self.src, 'common.h'), '')
end

function TestCppIncludes:tearDown()
  if fs.isdir(self.tmp) then
    fs.rmdir_r(self.tmp)
  end
end

function TestCppIncludes:make_processor()
  local processor = process_bind.new()
  processor:define('LLAE_BINDING_GENERATION')
  processor:define('META_OBJECT')
  processor:set_include_dirs({ self.src })
  processor:set_bind_src_dir(self.bind_src)
  return processor
end

function TestCppIncludes:test_include_on_namespace()
  local processor = self:make_processor()
  local result = processor:process_content([[
/// @luabind_include("mylib/real_api.h")
/// @luabind_include(<string>)
/// @luabind_include(mylib/api_extra.h)
namespace mylib {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]], path.join(self.src, 'mylib/api.h'))
  lu.assertEquals(result.modules.mylib:get_cpp_includes(), {
    '"../../src/mylib/real_api.h"',
    '<string>',
    '"../../src/mylib/api_extra.h"',
  })
end

function TestCppIncludes:test_include_on_class_is_deduplicated()
  local processor = self:make_processor()
  local result = processor:process_content([[
namespace mylib {
    /// @luabind_include("mylib/real_api.h")
    /// @luabind
    struct item {
        /// @luabind_include("mylib/real_api.h")
        /// @luabind
        int x;
        /// @luabind
        void method();
    };
}
]])
  lu.assertEquals(result.modules.mylib:get_cpp_includes(), { '"../../src/mylib/real_api.h"' })
end

function TestCppIncludes:test_no_include_tag()
  local processor = self:make_processor()
  local result = processor:process_content([[
namespace plain {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]])
  lu.assertEquals(result.modules.plain:get_cpp_includes(), {})
end

function TestCppIncludes:test_include_on_module_function()
  local processor = self:make_processor()
  local result = processor:process_content([[
/// @luabind_include("mylib/real_api.h")
/// @luabind(module=mylib)
void helper();
]])
  lu.assertEquals(result.modules.mylib:get_cpp_includes(), { '"../../src/mylib/real_api.h"' })
end

function TestCppIncludes:test_include_resolved_relative_to_header()
  local header = path.join(self.src, 'mylib/api.h')
  write_file(header, [[
#pragma once
/// @luabind_include("real_api.h")
/// @luabind_include("mylib/real_api.h")
namespace mylib {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]])
  local processor = self:make_processor()
  local result = processor:process_file(header)
  lu.assertEquals(result.modules.mylib:get_cpp_includes(), { '"../../src/mylib/real_api.h"' })
end

function TestCppIncludes:test_include_order_across_headers()
  local first = path.join(self.src, 'mylib/first.h')
  write_file(first, [[
#pragma once
/// @luabind_include("mylib/real_api.h")
namespace mylib {
    /// @luabind
    struct first {
        /// @luabind
        int x;
    };
}
]])
  local second = path.join(self.src, 'mylib/second.h')
  write_file(second, [[
#pragma once
/// @luabind_include("common.h")
/// @luabind_include("mylib/real_api.h")
namespace mylib {
    /// @luabind
    struct second {
        /// @luabind
        int y;
    };
}
]])
  local processor = self:make_processor()
  processor:process_file(first)
  local result = processor:process_file(second)
  lu.assertEquals(result.modules.mylib:get_cpp_includes(), {
    '"../../src/mylib/real_api.h"',
    '"../../src/common.h"',
  })
end

function TestCppIncludes:test_include_only_for_own_module()
  local alpha = path.join(self.src, 'alpha.h')
  write_file(alpha, [[
#pragma once
/// @luabind_include("common.h")
namespace alpha {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]])
  local beta = path.join(self.src, 'beta.h')
  write_file(beta, [[
#pragma once
namespace beta {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]])
  local processor = self:make_processor()
  processor:process_file(alpha)
  processor:process_file(beta)
  lu.assertEquals(processor:get_modules().alpha:get_cpp_includes(), { '"../../src/common.h"' })
  lu.assertEquals(processor:get_modules().beta:get_cpp_includes(), {})
end

function TestCppIncludes:test_include_not_found()
  local processor = self:make_processor()
  local ok, err = pcall(function()
    processor:process_content([[
/// @luabind_include("missing.h")
/// @luabind
namespace mylib {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]], path.join(self.src, 'mylib/api.h'))
  end)
  lu.assertFalse(ok)
  lu.assertStrContains(tostring(err), 'binding include not found')
  lu.assertStrContains(tostring(err), 'missing.h')
  lu.assertStrContains(tostring(err), 'search:')
end

function TestCppIncludes:test_include_empty_value()
  local processor = self:make_processor()
  for _,source in ipairs({
    [[
namespace mylib {
    /// @luabind_include
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]],
    [[
namespace mylib {
    /// @luabind_include()
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]],
    [[
namespace mylib {
    /// @luabind_include(named="x.h")
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]],
    [[
namespace mylib {
    /// @luabind_include("")
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]],
  }) do
    lu.assertErrorMsgContains('@luabind_include', function()
      processor:process_content(source)
    end)
  end
end

function TestCppIncludes:test_module_template_emits_includes()
  local processor = self:make_processor()
  local result = processor:process_content([[
/// @luabind_include("mylib/real_api.h")
/// @luabind_include(<string>)
namespace mylib {
    /// @luabind
    struct item {
        /// @luabind
        int x;
    };
}
]], path.join(self.src, 'mylib/api.h'))
  result.modules.mylib:add_header('../../src/mylib/api.h')

  local rendered = template.render_file(path.join(fs.pwd(), 'data', 'binding-module-template.cpp'), {
    escape = tostring,
    template = template,
    path = path,
    fs = fs,
    log = log,
    utils = utils,
    headers = {},
    module = result.modules.mylib,
  })

  local header_pos = rendered:find('#include "../../src/mylib/api.h"', 1, true)
  local include_pos = rendered:find('#include "../../src/mylib/real_api.h"', 1, true)
  local system_pos = rendered:find('#include <string>', 1, true)
  local class_pos = rendered:find('/* class mylib::item */', 1, true)
  lu.assertNotNil(header_pos)
  lu.assertNotNil(include_pos)
  lu.assertNotNil(system_pos)
  lu.assertNotNil(class_pos)
  lu.assertTrue(header_pos < include_pos)
  lu.assertTrue(include_pos < system_pos)
  lu.assertTrue(system_pos < class_pos)
end
