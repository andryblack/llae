local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local parser = require('cparse.parser')
package.path = store_path

local function parse(src)
  return parser.parse(src)
end

local function first(src)
  local ast = parse(src)
  return ast.children[1]
end

local function clean_text_join(t)
  local c = t:get_clean()
  if not c then
    return ''
  end
  return table.concat(c, '\n')
end

TestParserExternDocComment = {}

function TestParserExternDocComment:test_extern_doc_block_expands_to_real_decls()
  local src = [[
namespace ext {
  /** @extern
  enum class test_module_enum { a, b, c };
  struct test_bind_struct {
    int x = 0;
    enum class state {
      off,
      on,
    };
    state s;
  };
  */
}
  ]]
  local ns = first(src)
  lu.assertEquals(ns.kind, "namespace")
  lu.assertEquals(ns.name, "ext")
  lu.assertEquals(#ns.children, 2)

  local en = ns.children[1]
  lu.assertEquals(en.kind, "enum")
  lu.assertEquals(en.name, "test_module_enum")
  lu.assertTrue(en.tags:has("luabind"))

  local cls = ns.children[2]
  lu.assertEquals(cls.kind, "class")
  lu.assertEquals(cls.name, "test_bind_struct")
  lu.assertEquals(cls.struct_kind, "struct")
  lu.assertTrue(cls.tags:has("luabind"))
  lu.assertEquals(cls.children[1].kind, "field")
  lu.assertEquals(cls.children[1].name, "x")
  lu.assertTrue(cls.children[1].tags:has("luabind"))
  lu.assertEquals(cls.children[2].kind, "enum")
  lu.assertEquals(cls.children[2].name, "state")
  lu.assertTrue(cls.children[2].tags:has("luabind"))
  lu.assertEquals(cls.children[3].kind, "field")
  lu.assertEquals(cls.children[3].name, "s")
  lu.assertTrue(cls.children[3].tags:has("luabind"))
end

function TestParserExternDocComment:test_extern_doc_preserves_explicit_luabind()
  local src = [[
namespace ext {
  /** @extern
  /// @luabind(prefix=foo_)
  enum class E { a };
  */
}
  ]]
  local ns = first(src)
  local en = ns.children[1]
  local lb = en.tags:collect("luabind")
  lu.assertNotNil(lb)
  lu.assertEquals(lb.prefix, "foo_")
end

function TestParserExternDocComment:test_extern_field_directive_parses_as_fake_field()
  local src = [[
namespace ext {
  /** @extern
  struct S {
    @field(m, readonly=true) Mapped member field
    int keep;
  };
  */
}
  ]]
  local ns = first(src)
  local st = ns.children[1]
  lu.assertEquals(st.kind, "class")
  lu.assertEquals(st.name, "S")
  lu.assertEquals(st.children[1].kind, "field")
  lu.assertEquals(st.children[1].name, "m")
  lu.assertEquals(st.children[1].type, "unknown_binding_type")
  local lb = st.children[1].tags:collect("luabind")
  lu.assertNotNil(lb)
  lu.assertEquals(lb.readonly, "true")
  lu.assertEquals(st.children[1].tags:get_clean()[1], "Mapped member field")
  lu.assertEquals(st.children[2].kind, "field")
  lu.assertEquals(st.children[2].name, "keep")
end

function TestParserExternDocComment:test_extern_func_directive_parses_as_fake_function()
  local src = [[
namespace ext {
  /** @extern
  struct S {
    @func(reset, static=true)
    int keep;
  };
  */
}
  ]]
  local ns = first(src)
  local st = ns.children[1]
  lu.assertEquals(st.kind, "class")
  lu.assertEquals(st.name, "S")
  lu.assertEquals(st.children[1].kind, "function")
  lu.assertEquals(st.children[1].name, "reset")
  lu.assertEquals(st.children[1].return_type, "void")
  lu.assertEquals(#st.children[1].params, 0)
  local lb = st.children[1].tags:collect("luabind")
  lu.assertNotNil(lb)
  lu.assertEquals(lb.static, "true")
  lu.assertEquals(st.children[2].kind, "field")
  lu.assertEquals(st.children[2].name, "keep")
end

function TestParserExternDocComment:test_extern_field_at_namespace_is_constant_stub()
  local src = [[
namespace n {
  /** @extern
  @field(MAX_N, readonly=true)
  */
}
]]
  local ns = first(src)
  lu.assertEquals(ns.kind, "namespace")
  lu.assertEquals(ns.name, "n")
  lu.assertEquals(#ns.children, 1)
  local c = ns.children[1]
  lu.assertEquals(c.kind, "field")
  lu.assertEquals(c.name, "MAX_N")
  lu.assertEquals(c.type, "extern_constant")
  local lb = c.tags:collect("luabind")
  lu.assertNotNil(lb)
  lu.assertEquals(lb.readonly, "true")
end

function TestParserExternDocComment:test_extern_func_at_namespace_is_function_stub()
  local src = [[
namespace n {
  /** @extern
  @func(do_work, static=true)
  */
}
]]
  local ns = first(src)
  lu.assertEquals(#ns.children, 1)
  local f = ns.children[1]
  lu.assertEquals(f.kind, "function")
  lu.assertEquals(f.name, "do_work")
  lu.assertEquals(f.return_type, "void")
  lu.assertEquals(f.tags:collect("luabind").static, "true")
end

function TestParserExternDocComment:test_regular_doc_comment_after_extern_block()
  local src = [[
/** @extern
/// @luabind
enum class E { A };
*/
/// ordinary doc
void foo();
  ]]
  local ast = parse(src)
  lu.assertEquals(#ast.children, 2)
  lu.assertEquals(ast.children[1].kind, "enum")
  lu.assertEquals(ast.children[2].kind, "function")
  lu.assertStrContains(clean_text_join(ast.children[2].tags), "ordinary doc")
end
