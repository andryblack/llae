local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local lexer = require('cparse.lexer')
package.path = store_path

-- helpers

local function lex_all(src)
  local l = lexer.new(src)
  local tokens = {}
  while true do
    local tok = l:next()
    table.insert(tokens, tok)
    if tok.kind == "eof" then break end
  end
  return tokens
end

local function kinds(src)
  local result = {}
  for _, tok in ipairs(lex_all(src)) do
    if tok.kind ~= "eof" then
      table.insert(result, tok.kind)
    end
  end
  return result
end

local function values(src)
  local result = {}
  for _, tok in ipairs(lex_all(src)) do
    if tok.kind ~= "eof" then
      table.insert(result, tok.value)
    end
  end
  return result
end

local function lex_all_field_binding(src)
  local l = lexer.new(src, { allow_field_binding = true })
  local tokens = {}
  while true do
    local tok = l:next()
    table.insert(tokens, tok)
    if tok.kind == "eof" then break end
  end
  return tokens
end

local function kinds_field_binding(src)
  local result = {}
  for _, tok in ipairs(lex_all_field_binding(src)) do
    if tok.kind ~= "eof" then
      table.insert(result, tok.kind)
    end
  end
  return result
end

-- ============================================================
TestLexerConstructor = {}

function TestLexerConstructor:test_new_returns_lexer()
  local l = lexer.new("hello")
  lu.assertNotNil(l)
end

function TestLexerConstructor:test_empty_source_gives_eof()
  local l = lexer.new("")
  local tok = l:next()
  lu.assertEquals(tok.kind, "eof")
  lu.assertEquals(tok.value, "")
end

function TestLexerConstructor:test_eof_has_line()
  local l = lexer.new("")
  local tok = l:next()
  lu.assertEquals(tok.line, 1)
end

-- ============================================================
TestLexerKeywords = {}

function TestLexerKeywords:test_class()
  lu.assertEquals(kinds("class"), {"keyword"})
  lu.assertEquals(values("class"), {"class"})
end

function TestLexerKeywords:test_struct()
  lu.assertEquals(kinds("struct"), {"keyword"})
end

function TestLexerKeywords:test_union()
  lu.assertEquals(kinds("union"), {"keyword"})
end

function TestLexerKeywords:test_enum()
  lu.assertEquals(kinds("enum"), {"keyword"})
end

function TestLexerKeywords:test_namespace()
  lu.assertEquals(kinds("namespace"), {"keyword"})
end

function TestLexerKeywords:test_template()
  lu.assertEquals(kinds("template"), {"keyword"})
end

function TestLexerKeywords:test_typedef()
  lu.assertEquals(kinds("typedef"), {"keyword"})
end

function TestLexerKeywords:test_using()
  lu.assertEquals(kinds("using"), {"keyword"})
end

function TestLexerKeywords:test_extern()
  lu.assertEquals(kinds("extern"), {"keyword"})
end

function TestLexerKeywords:test_inline()
  lu.assertEquals(kinds("inline"), {"keyword"})
end

function TestLexerKeywords:test_static()
  lu.assertEquals(kinds("static"), {"keyword"})
end

function TestLexerKeywords:test_virtual()
  lu.assertEquals(kinds("virtual"), {"keyword"})
end

function TestLexerKeywords:test_const()
  lu.assertEquals(kinds("const"), {"keyword"})
end

function TestLexerKeywords:test_constexpr()
  lu.assertEquals(kinds("constexpr"), {"keyword"})
end

function TestLexerKeywords:test_override()
  lu.assertEquals(kinds("override"), {"keyword"})
end

function TestLexerKeywords:test_final()
  lu.assertEquals(kinds("final"), {"keyword"})
end

function TestLexerKeywords:test_public()
  lu.assertEquals(kinds("public"), {"keyword"})
end

function TestLexerKeywords:test_protected()
  lu.assertEquals(kinds("protected"), {"keyword"})
end

function TestLexerKeywords:test_private()
  lu.assertEquals(kinds("private"), {"keyword"})
end

function TestLexerKeywords:test_operator()
  lu.assertEquals(kinds("operator"), {"keyword"})
end

function TestLexerKeywords:test_typename()
  lu.assertEquals(kinds("typename"), {"keyword"})
end

function TestLexerKeywords:test_noexcept()
  lu.assertEquals(kinds("noexcept"), {"keyword"})
end

function TestLexerKeywords:test_explicit()
  lu.assertEquals(kinds("explicit"), {"keyword"})
end

function TestLexerKeywords:test_nullptr()
  lu.assertEquals(kinds("nullptr"), {"keyword"})
end

function TestLexerKeywords:test_true_false()
  lu.assertEquals(kinds("true"), {"keyword"})
  lu.assertEquals(kinds("false"), {"keyword"})
end

-- ============================================================
TestLexerIdents = {}

function TestLexerIdents:test_simple_ident()
  lu.assertEquals(kinds("foo"), {"ident"})
  lu.assertEquals(values("foo"), {"foo"})
end

function TestLexerIdents:test_ident_with_digits()
  lu.assertEquals(kinds("foo123"), {"ident"})
  lu.assertEquals(values("foo123"), {"foo123"})
end

function TestLexerIdents:test_underscore_ident()
  lu.assertEquals(kinds("_bar"), {"ident"})
  lu.assertEquals(values("_bar"), {"_bar"})
end

function TestLexerIdents:test_double_underscore()
  lu.assertEquals(kinds("__attribute__"), {"ident"})
end

function TestLexerIdents:test_keyword_prefix_not_keyword()
  -- "classes" is not a keyword
  lu.assertEquals(kinds("classes"), {"ident"})
  lu.assertEquals(values("classes"), {"classes"})
end

function TestLexerIdents:test_multiple_idents()
  lu.assertEquals(kinds("foo bar baz"), {"ident", "ident", "ident"})
  lu.assertEquals(values("foo bar baz"), {"foo", "bar", "baz"})
end

function TestLexerIdents:test_keyword_and_ident()
  lu.assertEquals(kinds("class Foo"), {"keyword", "ident"})
  lu.assertEquals(values("class Foo"), {"class", "Foo"})
end

-- ============================================================
TestLexerNumbers = {}

function TestLexerNumbers:test_integer()
  lu.assertEquals(kinds("42"), {"number"})
  lu.assertEquals(values("42"), {"42"})
end

function TestLexerNumbers:test_zero()
  lu.assertEquals(kinds("0"), {"number"})
  lu.assertEquals(values("0"), {"0"})
end

function TestLexerNumbers:test_hex()
  lu.assertEquals(kinds("0xFF"), {"number"})
  lu.assertEquals(values("0xFF"), {"0xFF"})
end

function TestLexerNumbers:test_hex_uppercase()
  lu.assertEquals(kinds("0XAB"), {"number"})
  lu.assertEquals(values("0XAB"), {"0XAB"})
end

function TestLexerNumbers:test_float()
  lu.assertEquals(kinds("3.14"), {"number"})
  lu.assertEquals(values("3.14"), {"3.14"})
end

function TestLexerNumbers:test_float_exponent()
  lu.assertEquals(kinds("1.5e10"), {"number"})
  lu.assertEquals(values("1.5e10"), {"1.5e10"})
end

function TestLexerNumbers:test_float_exponent_sign()
  lu.assertEquals(kinds("2.0E-3"), {"number"})
  lu.assertEquals(values("2.0E-3"), {"2.0E-3"})
end

function TestLexerNumbers:test_suffix_u()
  lu.assertEquals(kinds("42u"), {"number"})
  lu.assertEquals(values("42u"), {"42u"})
end

function TestLexerNumbers:test_suffix_ul()
  lu.assertEquals(kinds("42ul"), {"number"})
  lu.assertEquals(values("42ul"), {"42ul"})
end

function TestLexerNumbers:test_suffix_ull()
  lu.assertEquals(kinds("42ull"), {"number"})
  lu.assertEquals(values("42ull"), {"42ull"})
end

function TestLexerNumbers:test_suffix_f()
  lu.assertEquals(kinds("1.0f"), {"number"})
  lu.assertEquals(values("1.0f"), {"1.0f"})
end

-- ============================================================
TestLexerStrings = {}

function TestLexerStrings:test_empty_string()
  lu.assertEquals(kinds('""'), {"string"})
  lu.assertEquals(values('""'), {'""'})
end

function TestLexerStrings:test_simple_string()
  lu.assertEquals(kinds('"hello"'), {"string"})
  lu.assertEquals(values('"hello"'), {'"hello"'})
end

function TestLexerStrings:test_string_with_escape()
  lu.assertEquals(kinds('"he\\"llo"'), {"string"})
  lu.assertEquals(values('"he\\"llo"'), {'"he\\"llo"'})
end

function TestLexerStrings:test_string_with_backslash_n()
  lu.assertEquals(kinds('"line1\\nline2"'), {"string"})
end

function TestLexerStrings:test_string_token_kind()
  local toks = lex_all('"foo"')
  lu.assertEquals(toks[1].kind, "string")
end

-- ============================================================
TestLexerChars = {}

function TestLexerChars:test_simple_char()
  lu.assertEquals(kinds("'a'"), {"char"})
  lu.assertEquals(values("'a'"), {"'a'"})
end

function TestLexerChars:test_char_escape()
  lu.assertEquals(kinds("'\\n'"), {"char"})
  lu.assertEquals(values("'\\n'"), {"'\\n'"})
end

function TestLexerChars:test_char_null()
  lu.assertEquals(kinds("'\\0'"), {"char"})
end

-- ============================================================
TestLexerComments = {}

function TestLexerComments:test_line_comment_skipped()
  -- regular // comment yields no tokens
  lu.assertEquals(kinds("// this is a comment"), {})
end

function TestLexerComments:test_block_comment_skipped()
  lu.assertEquals(kinds("/* this is a comment */"), {})
end

function TestLexerComments:test_block_comment_multiline_skipped()
  local src = "/* line1\n   line2\n*/"
  lu.assertEquals(kinds(src), {})
end

function TestLexerComments:test_code_after_line_comment()
  lu.assertEquals(kinds("// comment\nfoo"), {"ident"})
  lu.assertEquals(values("// comment\nfoo"), {"foo"})
end

function TestLexerComments:test_code_around_block_comment()
  lu.assertEquals(kinds("int /* x */ y"), {"keyword", "ident"})
  lu.assertEquals(values("int /* x */ y"), {"int", "y"})
end

function TestLexerComments:test_doc_line_comment()
  lu.assertEquals(kinds("/// doc comment"), {"doc_comment"})
end

function TestLexerComments:test_doc_line_comment_value()
  local toks = lex_all("/// my doc")
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[1].value, "/// my doc")
end

function TestLexerComments:test_doc_block_comment()
  lu.assertEquals(kinds("/** doc block */"), {"doc_comment"})
end

function TestLexerComments:test_doc_block_comment_value()
  local toks = lex_all("/** my doc */")
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[1].value, "/** my doc */")
end

function TestLexerComments:test_doc_block_comment_multiline()
  local src = "/**\n * line1\n * line2\n */"
  lu.assertEquals(kinds(src), {"doc_comment"})
end

function TestLexerComments:test_empty_block_comment_not_doc()
  -- /**/ is NOT a doc comment (the '*' immediately closes it)
  lu.assertEquals(kinds("/**/"), {})
end

function TestLexerComments:test_doc_comment_before_decl()
  local src = "/// comment\nclass Foo"
  local toks = lex_all(src)
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[2].kind, "keyword")
  lu.assertEquals(toks[2].value, "class")
  lu.assertEquals(toks[3].kind, "ident")
  lu.assertEquals(toks[3].value, "Foo")
end

function TestLexerComments:test_regular_double_slash_not_triple()
  -- // is NOT a doc comment
  local toks = lex_all("// not doc")
  lu.assertEquals(toks[1].kind, "eof")
end

-- ============================================================
TestLexerFieldBinding = {}

function TestLexerFieldBinding:test_field_binding_not_lexed_by_default()
  lu.assertEquals(kinds("@field(x)"), { "punct", "ident", "punct", "ident", "punct" })
end

function TestLexerFieldBinding:test_field_binding_token_when_enabled()
  lu.assertEquals(kinds_field_binding("@field(x)"), { "field_binding" })
  local toks = lex_all_field_binding("@field(foo, readonly=true)")
  lu.assertEquals(toks[1].kind, "field_binding")
  lu.assertEquals(toks[1].value, "@field(foo, readonly=true)")
end

function TestLexerFieldBinding:test_field_binding_balanced_parens()
  local toks = lex_all_field_binding("@field(x, policy=field_ref_policy{})")
  lu.assertEquals(toks[1].kind, "field_binding")
  lu.assertStrContains(toks[1].value, "field_ref_policy{}")
end

function TestLexerFieldBinding:test_func_binding_token_when_enabled()
  lu.assertEquals(kinds_field_binding("@func(x)"), { "func_binding" })
  local toks = lex_all_field_binding("@func(reset, static=true)")
  lu.assertEquals(toks[1].kind, "func_binding")
  lu.assertEquals(toks[1].value, "@func(reset, static=true)")
end

function TestLexerFieldBinding:test_func_binding_balanced_parens()
  local toks = lex_all_field_binding("@func(x, policy=cb_policy{})")
  lu.assertEquals(toks[1].kind, "func_binding")
  lu.assertStrContains(toks[1].value, "cb_policy{}")
end

-- ============================================================
TestLexerPunct = {}

function TestLexerPunct:test_single_chars()
  local syms = {"{", "}", "(", ")", ";", ",", "<", ">", "=", "*", "&",
                "~", "!", "?", ":", "+", "-", "/", "%", "^", "|", "#", "."}
  for _, sym in ipairs(syms) do
    local toks = lex_all(sym)
    lu.assertEquals(toks[1].kind, "punct", "expected punct for: " .. sym)
    lu.assertEquals(toks[1].value, sym)
  end
end

function TestLexerPunct:test_scope_resolution()
  lu.assertEquals(kinds("::"), {"punct"})
  lu.assertEquals(values("::"), {"::"})
end

function TestLexerPunct:test_arrow()
  lu.assertEquals(kinds("->"), {"punct"})
  lu.assertEquals(values("->"), {"->"})
end

function TestLexerPunct:test_ellipsis()
  lu.assertEquals(kinds("..."), {"punct"})
  lu.assertEquals(values("..."), {"..."})
end

function TestLexerPunct:test_double_bracket_open()
  lu.assertEquals(kinds("[["), {"punct"})
  lu.assertEquals(values("[["), {"[["})
end

function TestLexerPunct:test_double_bracket_close()
  lu.assertEquals(kinds("]]"), {"punct"})
  lu.assertEquals(values("]]"), {"]]"})
end

function TestLexerPunct:test_eq_eq()
  lu.assertEquals(values("=="), {"=="})
end

function TestLexerPunct:test_not_eq()
  lu.assertEquals(values("!="), {"!="})
end

function TestLexerPunct:test_le()
  lu.assertEquals(values("<="), {"<="})
end

function TestLexerPunct:test_ge()
  lu.assertEquals(values(">="), {">="})
end

function TestLexerPunct:test_and_and()
  lu.assertEquals(values("&&"), {"&&"})
end

function TestLexerPunct:test_or_or()
  lu.assertEquals(values("||"), {"||"})
end

function TestLexerPunct:test_plus_plus()
  lu.assertEquals(values("++"), {"++"})
end

function TestLexerPunct:test_minus_minus()
  lu.assertEquals(values("--"), {"--"})
end

function TestLexerPunct:test_shift_left()
  lu.assertEquals(values("<<"), {"<<"})
end

function TestLexerPunct:test_shift_right()
  lu.assertEquals(values(">>"), {">>"})
end

function TestLexerPunct:test_shift_left_assign()
  lu.assertEquals(values("<<="), {"<<="})
end

function TestLexerPunct:test_shift_right_assign()
  lu.assertEquals(values(">>="), {">>="})
end

function TestLexerPunct:test_assign_variants()
  lu.assertEquals(values("+="), {"+="})
  lu.assertEquals(values("-="), {"-="})
  lu.assertEquals(values("*="), {"*="})
  lu.assertEquals(values("/="), {"/="})
  lu.assertEquals(values("%="), {"%="})
  lu.assertEquals(values("&="), {"&="})
  lu.assertEquals(values("|="), {"|="})
  lu.assertEquals(values("^="), {"^="})
end

-- ============================================================
TestLexerLines = {}

function TestLexerLines:test_line_tracking_single()
  local toks = lex_all("foo")
  lu.assertEquals(toks[1].line, 1)
end

function TestLexerLines:test_line_tracking_multiline()
  local toks = lex_all("foo\nbar\nbaz")
  lu.assertEquals(toks[1].line, 1)
  lu.assertEquals(toks[2].line, 2)
  lu.assertEquals(toks[3].line, 3)
end

function TestLexerLines:test_line_tracking_after_comment()
  local toks = lex_all("// comment\nfoo")
  lu.assertEquals(toks[1].line, 2)
end

function TestLexerLines:test_line_tracking_after_block_comment()
  local src = "/* line1\n   line2\n*/foo"
  local toks = lex_all(src)
  lu.assertEquals(toks[1].line, 3)
end

function TestLexerLines:test_line_tracking_doc_comment()
  local src = "\n/// doc\nfoo"
  local toks = lex_all(src)
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[1].line, 2)
  lu.assertEquals(toks[2].line, 3)
end

-- ============================================================
TestLexerAPI = {}

function TestLexerAPI:test_peek_does_not_consume()
  local l = lexer.new("foo bar")
  local t1 = l:peek()
  local t2 = l:peek()
  lu.assertEquals(t1.value, t2.value)
  lu.assertEquals(t1.value, "foo")
  -- next should still return "foo"
  local t3 = l:next()
  lu.assertEquals(t3.value, "foo")
end

function TestLexerAPI:test_next_consumes()
  local l = lexer.new("foo bar")
  l:next()
  local tok = l:next()
  lu.assertEquals(tok.value, "bar")
end

function TestLexerAPI:test_expect_success()
  local l = lexer.new("foo")
  local tok = l:expect("ident")
  lu.assertEquals(tok.value, "foo")
end

function TestLexerAPI:test_expect_with_value()
  local l = lexer.new("class")
  local tok = l:expect("keyword", "class")
  lu.assertEquals(tok.value, "class")
end

function TestLexerAPI:test_expect_failure_kind()
  local l = lexer.new("foo")
  lu.assertErrorMsgContains("expected [keyword]", function()
    l:expect("keyword")
  end)
end

function TestLexerAPI:test_expect_failure_value()
  local l = lexer.new("class")
  lu.assertErrorMsgContains("expected [keyword 'struct']", function()
    l:expect("keyword", "struct")
  end)
end

function TestLexerAPI:test_accept_match()
  local l = lexer.new("foo")
  local tok = l:accept("ident")
  lu.assertNotNil(tok)
  lu.assertEquals(tok.value, "foo")
end

function TestLexerAPI:test_accept_no_match()
  local l = lexer.new("foo")
  local tok = l:accept("keyword")
  lu.assertNil(tok)
  -- token not consumed, next should still return "foo"
  lu.assertEquals(l:next().value, "foo")
end

function TestLexerAPI:test_accept_with_value_match()
  local l = lexer.new("class")
  lu.assertNotNil(l:accept("keyword", "class"))
end

function TestLexerAPI:test_accept_with_value_no_match()
  local l = lexer.new("class")
  lu.assertNil(l:accept("keyword", "struct"))
  lu.assertEquals(l:next().value, "class")
end

function TestLexerAPI:test_multiple_eof_calls()
  local l = lexer.new("")
  lu.assertEquals(l:next().kind, "eof")
  lu.assertEquals(l:next().kind, "eof")
  lu.assertEquals(l:next().kind, "eof")
end

-- ============================================================
TestLexerCppSnippets = {}

function TestLexerCppSnippets:test_class_decl()
  -- class Foo : public Bar { };
  local src = "class Foo : public Bar { };"
  local v = values(src)
  lu.assertEquals(v, {"class", "Foo", ":", "public", "Bar", "{", "}", ";"})
  local k = kinds(src)
  lu.assertEquals(k, {"keyword", "ident", "punct", "keyword", "ident", "punct", "punct", "punct"})
end

function TestLexerCppSnippets:test_namespace_decl()
  local src = "namespace Foo { }"
  lu.assertEquals(values(src), {"namespace", "Foo", "{", "}"})
end

function TestLexerCppSnippets:test_function_decl()
  local src = "int doThing(int x, float y);"
  local v = values(src)
  lu.assertEquals(v, {"int", "doThing", "(", "int", "x", ",", "float", "y", ")", ";"})
end

function TestLexerCppSnippets:test_template_decl()
  local src = "template<typename T>"
  lu.assertEquals(values(src), {"template", "<", "typename", "T", ">"})
end

function TestLexerCppSnippets:test_scope_resolution()
  local src = "std::vector"
  lu.assertEquals(values(src), {"std", "::", "vector"})
  lu.assertEquals(kinds(src), {"ident", "punct", "ident"})
end

function TestLexerCppSnippets:test_pointer_and_ref()
  local src = "int* ptr = &val;"
  lu.assertEquals(values(src), {"int", "*", "ptr", "=", "&", "val", ";"})
end

function TestLexerCppSnippets:test_destructor()
  local src = "~Foo();"
  lu.assertEquals(values(src), {"~", "Foo", "(", ")", ";"})
end

function TestLexerCppSnippets:test_attribute()
  local src = "[[nodiscard]] int get();"
  lu.assertEquals(values(src), {"[[", "nodiscard", "]]", "int", "get", "(", ")", ";"})
end

function TestLexerCppSnippets:test_pure_virtual()
  local src = "virtual void foo() = 0;"
  lu.assertEquals(values(src), {"virtual", "void", "foo", "(", ")", "=", "0", ";"})
end

function TestLexerCppSnippets:test_extern_c()
  local src = 'extern "C" {'
  lu.assertEquals(values(src), {"extern", '"C"', "{"})
  lu.assertEquals(kinds(src), {"keyword", "string", "punct"})
end

function TestLexerCppSnippets:test_using_alias()
  local src = "using MyInt = int;"
  lu.assertEquals(values(src), {"using", "MyInt", "=", "int", ";"})
end

function TestLexerCppSnippets:test_enum_class()
  local src = "enum class Color { Red, Green, Blue };"
  lu.assertEquals(values(src), {"enum", "class", "Color", "{", "Red", ",", "Green", ",", "Blue", "}", ";"})
end

function TestLexerCppSnippets:test_doc_comment_attached()
  local src = "/** Get value */\nint getValue();"
  local toks = lex_all(src)
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[2].kind, "keyword")  -- int
  lu.assertEquals(toks[3].kind, "ident")    -- getValue
end

function TestLexerCppSnippets:test_pragma_annotation()
  -- preprocessor emits pragmas as /// @pragma(...)
  local src = "/// @pragma( once )"
  local toks = lex_all(src)
  lu.assertEquals(toks[1].kind, "doc_comment")
  lu.assertEquals(toks[1].value, "/// @pragma( once )")
end
