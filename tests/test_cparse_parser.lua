local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local parser = require('cparse.parser')
local preprocessor = require('cparse.preprocessor')
package.path = store_path

-- helpers

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

-- ============================================================
TestParserBasic = {}

function TestParserBasic:test_empty_source()
  local ast = parse("")
  lu.assertEquals(ast.kind, "file")
  lu.assertEquals(#ast.children, 0)
end

function TestParserBasic:test_file_kind()
  local ast = parse("int x;")
  lu.assertEquals(ast.kind, "file")
end

function TestParserBasic:test_semicolon_only()
  local ast = parse(";")
  lu.assertEquals(#ast.children, 0)
end

function TestParserBasic:test_multiple_semis()
  local ast = parse(";;;")
  lu.assertEquals(#ast.children, 0)
end

-- ============================================================
TestParserNamespace = {}

function TestParserNamespace:test_empty_namespace()
  local n = first("namespace Foo {}")
  lu.assertEquals(n.kind, "namespace")
  lu.assertEquals(n.name, "Foo")
  lu.assertEquals(#n.children, 0)
end

function TestParserNamespace:test_namespace_with_decl()
  local ast = parse("namespace Foo { int x; }")
  local n = ast.children[1]
  lu.assertEquals(n.kind, "namespace")
  lu.assertEquals(n.name, "Foo")
  lu.assertEquals(#n.children, 1)
end

function TestParserNamespace:test_anonymous_namespace()
  local n = first("namespace {}")
  lu.assertEquals(n.kind, "namespace")
  lu.assertEquals(n.name, "")
end

function TestParserNamespace:test_nested_namespace()
  local n = first("namespace A { namespace B {} }")
  lu.assertEquals(n.kind, "namespace")
  lu.assertEquals(n.name, "A")
  lu.assertEquals(n.children[1].name, "B")
end

-- ============================================================
TestParserClass = {}

function TestParserClass:test_empty_class()
  local n = first("class Foo {};")
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.name, "Foo")
  lu.assertEquals(n.struct_kind, "class")
end

function TestParserClass:test_empty_struct()
  local n = first("struct Bar {};")
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.struct_kind, "struct")
  lu.assertEquals(n.name, "Bar")
end

function TestParserClass:test_empty_union()
  local n = first("union U {};")
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.struct_kind, "union")
end

function TestParserClass:test_forward_decl()
  local n = first("class Foo;")
  lu.assertEquals(n.kind, "class_forward")
  lu.assertEquals(n.name, "Foo")
end

function TestParserClass:test_single_inheritance()
  local n = first("class Foo : public Bar {};")
  lu.assertEquals(#n.bases, 1)
  lu.assertEquals(n.bases[1].name, "Bar")
  lu.assertEquals(n.bases[1].access, "public")
end

function TestParserClass:test_multiple_inheritance()
  local n = first("class Foo : public A, protected B {};")
  lu.assertEquals(#n.bases, 2)
  lu.assertEquals(n.bases[1].name, "A")
  lu.assertEquals(n.bases[2].name, "B")
  lu.assertEquals(n.bases[2].access, "protected")
end

function TestParserClass:test_scoped_base()
  local n = first("class Foo : public ns::Base {};")
  lu.assertEquals(n.bases[1].name, "ns::Base")
end

function TestParserClass:test_class_with_field()
  local n = first("struct S { int x; };")
  lu.assertEquals(#n.children, 1)
  lu.assertEquals(n.children[1].kind, "field")
  lu.assertEquals(n.children[1].name, "x")
end

function TestParserClass:test_class_with_method()
  local n = first("class C { void foo(); };")
  lu.assertEquals(n.children[1].kind, "function")
  lu.assertEquals(n.children[1].name, "foo")
end

function TestParserClass:test_class_with_constructor()
  local n = first("class C { C(); };")
  lu.assertEquals(n.children[1].kind, "function")
  lu.assertEquals(n.children[1].name, "C")
end

function TestParserClass:test_class_with_constructor2()
  local n = first[[
/// @luabind
    struct test_bind_fields {
        static size_t count;
        /// @luabind(raw=true)
        test_bind_fields();
        ~test_bind_fields();
    };
]]
  lu.assertEquals(n.children[2].kind, "function")
  lu.assertEquals(n.children[2].name, "test_bind_fields")
  lu.assertTrue(n.children[2].tags:has("luabind"))
end



function TestParserClass:test_access_specifier_node()
  local n = first("class C { public: int x; };")
  lu.assertEquals(n.children[1].kind, "access")
  lu.assertEquals(n.children[1].access, "public")
  lu.assertEquals(n.children[2].kind, "field")
end

function TestParserClass:test_nested_class()
  local n = first("class Outer { class Inner {}; };")
  lu.assertEquals(n.children[1].kind, "class")
  lu.assertEquals(n.children[1].name, "Inner")
end

-- ============================================================
TestParserEnum = {}

function TestParserEnum:test_simple_enum()
  local n = first("enum Color { Red, Green, Blue };")
  lu.assertEquals(n.kind, "enum")
  lu.assertEquals(n.name, "Color")
  lu.assertFalse(n.is_scoped)
  lu.assertEquals(#n.values, 3)
  lu.assertEquals(n.values[1].name, "Red")
  lu.assertEquals(n.values[2].name, "Green")
  lu.assertEquals(n.values[3].name, "Blue")
end

function TestParserEnum:test_enum_class()
  local n = first("enum class Status { Ok, Err };")
  lu.assertEquals(n.kind, "enum")
  lu.assertTrue(n.is_scoped)
  lu.assertEquals(n.name, "Status")
end

function TestParserEnum:test_enum_with_values()
  local n = first("enum E { A = 1, B = 2, C };")
  lu.assertEquals(n.values[1].value, "1")
  lu.assertEquals(n.values[2].value, "2")
  lu.assertNil(n.values[3].value)
end

function TestParserEnum:test_enum_base_type()
  local n = first("enum class E : uint8_t { A };")
  lu.assertEquals(n.base_type, "uint8_t")
end

function TestParserEnum:test_enum_forward_decl()
  local n = first("enum class E : int;")
  lu.assertTrue(n.forward)
end

-- ============================================================
TestParserFunction = {}

function TestParserFunction:test_simple_function()
  local n = first("void foo();")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "foo")
  lu.assertEquals(n.return_type, "void")
end

function TestParserFunction:test_int_return()
  local n = first("int getValue();")
  lu.assertEquals(n.return_type, "int")
  lu.assertEquals(n.name, "getValue")
end

function TestParserFunction:test_no_params()
  local n = first("void f();")
  lu.assertEquals(#n.params, 0)
end

function TestParserFunction:test_void_params()
  local n = first("void f(void);")
  lu.assertEquals(#n.params, 0)
end

function TestParserFunction:test_one_param()
  local n = first("void f(int x);")
  lu.assertEquals(#n.params, 1)
  lu.assertEquals(n.params[1].type, "int")
  lu.assertEquals(n.params[1].name, "x")
end

function TestParserFunction:test_two_params()
  local n = first("void f(int x, float y);")
  lu.assertEquals(#n.params, 2)
  lu.assertEquals(n.params[1].name, "x")
  lu.assertEquals(n.params[2].name, "y")
end

function TestParserFunction:test_unnamed_param()
  local n = first("void f(int);")
  lu.assertEquals(#n.params, 1)
  lu.assertEquals(n.params[1].type, "int")
end

function TestParserFunction:test_default_param()
  local n = first("void f(int x = 0);")
  lu.assertEquals(n.params[1].default, "0")
end

function TestParserFunction:test_variadic()
  local n = first("void f(int x, ...);")
  lu.assertEquals(n.params[2].type, "...")
end

function TestParserFunction:test_const_method()
  local n = first("class C { int get() const; };")
  lu.assertTrue(n.children[1].qualifiers.const)
end

function TestParserFunction:test_virtual_method()
  local n = first("class C { virtual void foo(); };")
  lu.assertTrue(n.children[1].qualifiers.virtual)
end

function TestParserFunction:test_pure_virtual()
  local n = first("class C { virtual void foo() = 0; };")
  lu.assertTrue(n.children[1].qualifiers.pure)
end

function TestParserFunction:test_override()
  local n = first("class C { void foo() override; };")
  lu.assertTrue(n.children[1].qualifiers.override)
end

function TestParserFunction:test_deleted()
  local n = first("class C { C(const C&) = delete; };")
  lu.assertTrue(n.children[1].qualifiers.deleted)
end

function TestParserFunction:test_defaulted()
  local n = first("class C { C() = default; };")
  lu.assertTrue(n.children[1].qualifiers.defaulted)
end

function TestParserFunction:test_static_method()
  local n = first("class C { static int count(); };")
  lu.assertTrue(n.children[1].qualifiers.static)
end

function TestParserFunction:test_noexcept()
  local n = first("void f() noexcept;")
  lu.assertTrue(n.qualifiers.noexcept)
end

function TestParserFunction:test_noexcept_expr()
  local n = first("void f() noexcept(true);")
  lu.assertTrue(n.qualifiers.noexcept)
end

function TestParserFunction:test_destructor()
  local n = first("class C { ~C(); };")
  lu.assertEquals(n.children[1].kind, "function")
  lu.assertEquals(n.children[1].name, "~C")
end

function TestParserFunction:test_pointer_return()
  local n = first("int* getPtr();")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "getPtr")
end

function TestParserFunction:test_ref_return()
  local n = first("const std::string& getName();")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "getName")
end

function TestParserFunction:test_inline_body_skipped()
  -- inline function definition in a class — body should be skipped
  local n = first("class C { int f() { return 42; } };")
  lu.assertEquals(n.children[1].kind, "function")
  lu.assertEquals(n.children[1].name, "f")
end

function TestParserFunction:test_operator_overload()
  local n = first("class C { bool operator==(const C& o) const; };")
  local fn = n.children[1]
  lu.assertEquals(fn.kind, "function")
  -- name starts with "operator"
  lu.assertStrContains(fn.name, "operator")
end

-- ============================================================
TestParserField = {}

function TestParserField:test_simple_field()
  local n = first("struct S { int x; };")
  local f = n.children[1]
  lu.assertEquals(f.kind, "field")
  lu.assertEquals(f.name, "x")
  lu.assertEquals(f.type, "int")
end

function TestParserField:test_field_with_initializer()
  local n = first("struct S { int x = 0; };")
  lu.assertEquals(n.children[1].kind, "field")
  lu.assertEquals(n.children[1].name, "x")
end

function TestParserField:test_static_field()
  local n = first("struct S { static int count; };")
  lu.assertTrue(n.children[1].qualifiers.static)
end

function TestParserField:test_const_field()
  local n = first("struct S { const int max; };")
  lu.assertEquals(n.children[1].kind, "field")
end

function TestParserField:test_pointer_field()
  local n = first("struct S { int* ptr; };")
  lu.assertEquals(n.children[1].kind, "field")
  lu.assertEquals(n.children[1].name, "ptr")
end

function TestParserField:test_top_level_var()
  local n = first("int globalVar;")
  lu.assertEquals(n.kind, "field")
  lu.assertEquals(n.name, "globalVar")
end

-- ============================================================
TestParserTypedef = {}

function TestParserTypedef:test_simple_typedef()
  local n = first("typedef int MyInt;")
  lu.assertEquals(n.kind, "typedef")
  lu.assertEquals(n.name, "MyInt")
  lu.assertEquals(n.type, "int")
end

function TestParserTypedef:test_typedef_pointer()
  local n = first("typedef void* Handle;")
  lu.assertEquals(n.kind, "typedef")
  lu.assertEquals(n.name, "Handle")
end

function TestParserTypedef:test_typedef_struct()
  local src = "typedef struct { int x; } Point;"
  local n = first(src)
  lu.assertEquals(n.kind, "typedef")
  lu.assertEquals(n.name, "Point")
  lu.assertNotNil(n.inner)
  lu.assertEquals(n.inner.kind, "class")
end

-- ============================================================
TestParserUsing = {}

function TestParserUsing:test_using_alias()
  local n = first("using MyInt = int;")
  lu.assertEquals(n.kind, "using")
  lu.assertEquals(n.name, "MyInt")
  lu.assertEquals(n.type, "int")
end

function TestParserUsing:test_using_namespace()
  local n = first("using namespace std;")
  lu.assertEquals(n.kind, "using_namespace")
  lu.assertEquals(n.name, "std")
end

function TestParserUsing:test_using_scoped_namespace()
  local n = first("using namespace foo::bar;")
  lu.assertEquals(n.kind, "using_namespace")
  lu.assertEquals(n.name, "foo::bar")
end

function TestParserUsing:test_using_template_alias()
  local n = first("using Vec = std::vector<int>;")
  lu.assertEquals(n.kind, "using")
  lu.assertEquals(n.name, "Vec")
  lu.assertNotNil(n.type)
end

-- ============================================================
TestParserTemplate = {}

function TestParserTemplate:test_template_class()
  local n = first("template<typename T> class Foo {};")
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.name, "Foo")
  lu.assertNotNil(n.template_params)
  lu.assertStrContains(n.template_params, "template")
  lu.assertStrContains(n.template_params, "T")
end

function TestParserTemplate:test_template_function()
  local n = first("template<typename T> T max(T a, T b);")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "max")
  lu.assertNotNil(n.template_params)
end

function TestParserTemplate:test_template_multiparams()
  local n = first("template<typename K, typename V> class Map {};")
  lu.assertEquals(n.kind, "class")
  lu.assertStrContains(n.template_params, "K")
  lu.assertStrContains(n.template_params, "V")
end

-- ============================================================
TestParserExternC = {}

function TestParserExternC:test_extern_c_block()
  local n = first('extern "C" { void foo(); }')
  lu.assertEquals(n.kind, "extern_block")
  lu.assertEquals(n.linkage, '"C"')
  lu.assertEquals(#n.children, 1)
  lu.assertEquals(n.children[1].name, "foo")
end

function TestParserExternC:test_extern_c_single()
  local n = first('extern "C" void foo();')
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "foo")
  lu.assertEquals(n.extern_linkage, '"C"')
end

-- ============================================================
TestParserDocComment = {}

function TestParserDocComment:test_doc_attached_to_function()
  local src = "/// Get the value\nvoid getValue();"
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertStrContains(clean_text_join(n.tags), "Get the value")
end

function TestParserDocComment:test_doc_attached_to_class()
  local src = "/** My class */\nclass Foo {};"
  local n = first(src)
  lu.assertEquals(n.kind, "class")
  lu.assertStrContains(clean_text_join(n.tags), "My class")
end

function TestParserDocComment:test_doc_attached_to_field()
  local src = "struct S { /// field doc\nint x; };"
  local n = first(src)
  local f = n.children[1]
  lu.assertStrContains(clean_text_join(f.tags), "field doc")
end

function TestParserDocComment:test_doc_attached_to_namespace()
  local src = "/// ns doc\nnamespace Foo {}"
  local n = first(src)
  lu.assertNotNil(n.tags:get_clean())
end

function TestParserDocComment:test_regular_comment_not_doc()
  -- preprocessor would have stripped regular // comments, but if the
  -- lexer is fed raw source, regular // comments are just skipped
  local src = "// regular\nvoid f();"
  local n = first(src)
  lu.assertNil(n.tags:get_clean())
  lu.assertFalse(n.tags:has("luabind"))
end

-- ============================================================
TestParserComplex = {}

function TestParserComplex:test_class_with_all_members()
  local src = [[
class MyClass : public Base {
public:
  MyClass();
  ~MyClass();
  int getValue() const;
  void setValue(int v);
  static int count();
  virtual void onEvent() = 0;
private:
  int value_;
  static int count_;
};
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.name, "MyClass")
  lu.assertEquals(#n.bases, 1)
  -- just check it parsed without error
  lu.assertTrue(#n.children > 1)
  lu.assertEquals(n.children[1].kind, "access")
  lu.assertEquals(n.children[2].kind, "function")
  lu.assertEquals(n.children[2].name, "MyClass")
end

function TestParserComplex:test_namespace_with_class()
  local src = [[
namespace myns {
  class Foo {
  public:
    void bar();
  };
}
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "namespace")
  lu.assertEquals(n.name, "myns")
  lu.assertEquals(n.children[1].kind, "class")
  lu.assertEquals(n.children[1].name, "Foo")
end

function TestParserComplex:test_template_class_with_method()
  local src = [[
template<typename T>
class Container {
public:
  void push(T val);
  T pop();
  int size() const;
};
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "class")
  lu.assertEquals(n.name, "Container")
  lu.assertNotNil(n.template_params)
  lu.assertTrue(#n.children > 0)
end

function TestParserComplex:test_multiple_top_level_decls()
  local src = [[
namespace A {}
class B {};
void f();
int x;
  ]]
  local ast = parse(src)
  lu.assertEquals(#ast.children, 4)
  lu.assertEquals(ast.children[1].kind, "namespace")
  lu.assertEquals(ast.children[2].kind, "class")
  lu.assertEquals(ast.children[3].kind, "function")
  lu.assertEquals(ast.children[4].kind, "field")
end

function TestParserComplex:test_enum_class_in_namespace()
  local src = [[
namespace gfx {
  enum class Color : int { Red = 0, Green, Blue };
}
  ]]
  local ns = first(src)
  lu.assertEquals(ns.kind, "namespace")
  local en = ns.children[1]
  lu.assertEquals(en.kind, "enum")
  lu.assertTrue(en.is_scoped)
  lu.assertEquals(en.base_type, "int")
  lu.assertEquals(#en.values, 3)
end

function TestParserComplex:test_extern_c_in_header()
  local src = [[
#ifdef __cplusplus
extern "C" {
#endif
void c_function(int x);
int c_other();
#ifdef __cplusplus
}
#endif
  ]]
  -- preprocessor strips #ifdef, but we test the parser with raw C++ output
  -- simulate what preprocessor would emit for extern "C" block:
  local src2 = 'extern "C" { void c_function(int x); int c_other(); }'
  local n = first(src2)
  lu.assertEquals(n.kind, "extern_block")
  lu.assertEquals(#n.children, 2)
end

function TestParserComplex:test_attribute_nodiscard()
  local src = "[[nodiscard]] int compute();"
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "compute")
  lu.assertNotNil(n.attributes)
  lu.assertStrContains(n.attributes, "nodiscard")
end

function TestParserComplex:test_ref_param()
  local src = "void swap(int& a, int& b);"
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(#n.params, 2)
end

function TestParserComplex:test_const_ref_param()
  local src = "void print(const std::string& s);"
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(#n.params, 1)
end

function TestParserComplex:test_ctr_init_list()
  local src = [[
class compress_buffers {
  compress_buffers(compressionstream_ptr&& stream,llae::write_buffers&& buffers) : compress_work(std::move(stream)),m_buffers(std::move(buffers)) {  
  }
};
]]
  local n = first(src)
end

function TestParserComplex:test_freeze()
  -- local fs = require 'llae.fs'
  -- local async = require 'llae.async'
  -- local log = require 'llae.log'
  -- local src = fs.load_file('src/archive/common.h')
  -- local pp = preprocessor.new()
  -- local result = pp:process(src)
  -- local th = coroutine.running()
  -- -- parser._yield = function()
  -- --   async.pause(1)
  -- -- end
  -- local p = parser.new(result)
  -- -- async.run(function()
  -- --   while true do
  -- --     async.pause(1000)
  -- --     p:dump()
  -- --     log.info('freeze',debug.traceback(th))
  -- --   end
  -- -- end)
  -- local ast = p:_parse_impl()
  -- lu.assertNotNil(ast)
end

TestParserDocs = {}

function TestParserDocs:test_doc_function()
  local src = [[
/// Get the value
void getValue();
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertStrContains(clean_text_join(n.tags), "Get the value")
end

function TestParserDocs:test_doc_function_multi()
  local src = [[
/// Get the value
/// Is a function
void getValue();
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "function")
  lu.assertStrContains(clean_text_join(n.tags), "Get the value")
  lu.assertStrContains(clean_text_join(n.tags), "Is a function")
end

function TestParserDocs:test_doc_class()
  local src = [[
/// Is a class
class MyClass {
  public:
    MyClass();
    ~MyClass();
};
  ]]
  local n = first(src)
  lu.assertEquals(n.kind, "class")
  lu.assertStrContains(clean_text_join(n.tags), "Is a class")
end

TestParserFiles = {}

function TestParserFiles:test_include_file()
  -- --if true then return end
  --   local pp = preprocessor.new()
  --   local fs = require 'llae.fs'
  --   local path = require 'llae.path'
  --   local log = require 'llae.log'
  --   for _,v in ipairs(fs.scanfiles_r('src')) do
  --       local ext = path.extension(v)
  --       if ext == 'h' then
  --           log.info('process',v)
  --           local content = tostring(fs.load_file(path.join('src',v)))
  --           local result = pp:process(content)
  --           lu.assertNotNil(result)
  --           log.info('parse',v)
  --           local ast = parser.parse(result)
  --           lu.assertNotNil(ast)
  --           --log.info('result',result)
  --       end
  --   end
end

TestParserArgs = {}

function TestParserArgs:test_template_multiparams()
  local n = first("void foo(int,float);")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "foo")
  lu.assertEquals(#n.params, 2)
  lu.assertEquals(n.params[1].type, "int")
  lu.assertEquals(n.params[2].type, "float")
end

function TestParserArgs:test_template_multiparams()
  local n = first("void foo(std::string);")
  lu.assertEquals(n.kind, "function")
  lu.assertEquals(n.name, "foo")
  lu.assertEquals(#n.params, 1)
  lu.assertEquals(n.params[1].type, "std::string")
end
