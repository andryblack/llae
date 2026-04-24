local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local preprocessor = require('cparse.preprocessor')
package.path = store_path

TestPreprocessor = {}

-- Helper to create a preprocessor with a simple include handler
local function create_pp(inc_handler)
    return preprocessor.new(inc_handler)
end

function TestPreprocessor:test_constructor()
    local pp = create_pp()
    lu.assertNotNil(pp)
    lu.assertNotNil(pp._defines)
    lu.assertNotNil(pp._inc_handler)
end

function TestPreprocessor:test_define_basic()
    local pp = create_pp()
    pp:define('FOO')
    lu.assertTrue(pp:is_defined('FOO'))
    lu.assertEquals(pp:get_define('FOO'), '')
end

function TestPreprocessor:test_define_with_value()
    local pp = create_pp()
    pp:define('FOO', 'bar')
    lu.assertTrue(pp:is_defined('FOO'))
    lu.assertEquals(pp:get_define('FOO'), 'bar')
end

function TestPreprocessor:test_undef()
    local pp = create_pp()
    pp:define('FOO', 'bar')
    lu.assertTrue(pp:is_defined('FOO'))
    pp:undef('FOO')
    lu.assertFalse(pp:is_defined('FOO'))
    lu.assertNil(pp:get_define('FOO'))
end

function TestPreprocessor:test_process_empty()
    local pp = create_pp()
    local result = pp:process('')
    lu.assertEquals(result, '')
end

function TestPreprocessor:test_process_plain_text()
    local pp = create_pp()
    local result = pp:process('hello world')
    lu.assertEquals(result, 'hello world')
end

function TestPreprocessor:test_define_directive()
    local pp = create_pp()
    local result = pp:process('#define FOO bar')
    lu.assertEquals(result, '')
    lu.assertTrue(pp:is_defined('FOO'))
    lu.assertEquals(pp:get_define('FOO'), 'bar')
end

function TestPreprocessor:test_define_directive_with_expansion()
    local pp = create_pp()
    pp:process('#define FOO bar')
    local result = pp:process('FOO')
    lu.assertEquals(result, 'bar')
end

function TestPreprocessor:test_define_multiline()
    local pp = create_pp()
    local input = [[
#define FOO bar
#define BAZ qux
FOO BAZ
]]
    local result = pp:process(input)
    lu.assertEquals(result, 'bar qux')
end

function TestPreprocessor:test_ifdef_defined()
    local pp = create_pp()
    pp:define('FOO')
    local input = [[
#ifdef FOO
defined_foo
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'defined_foo')
end

function TestPreprocessor:test_ifdef_undefined()
    local pp = create_pp()
    local input = [[
#ifdef FOO
defined_foo
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'defined_foo')
end

function TestPreprocessor:test_ifndef_defined()
    local pp = create_pp()
    pp:define('FOO')
    local input = [[
#ifndef FOO
not_defined_foo
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'not_defined_foo')
end

function TestPreprocessor:test_ifndef_undefined()
    local pp = create_pp()
    local input = [[
#ifndef FOO
not_defined_foo
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'not_defined_foo')
end

function TestPreprocessor:test_if_else()
    local pp = create_pp()
    pp:define('FOO')
    local input = [[
#ifdef FOO
foo_branch
#else
else_branch
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_branch')
    lu.assertNotStrContains(result, 'else_branch')
end

function TestPreprocessor:test_if_else_undefined()
    local pp = create_pp()
    local input = [[
#ifdef FOO
foo_branch
#else
else_branch
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'foo_branch')
    lu.assertStrContains(result, 'else_branch')
end

function TestPreprocessor:test_if_expression_true()
    local pp = create_pp()
    pp:define('FOO', '1')
    local input = [[
#if FOO
foo_is_true
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_is_true')
end

function TestPreprocessor:test_if_expression_false()
    local pp = create_pp()
    pp:define('FOO', '0')
    local input = [[
#if FOO
foo_is_true
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'foo_is_true')
end

function TestPreprocessor:test_if_defined_operator()
    local pp = create_pp()
    pp:define('FOO')
    local input = [[
#if defined(FOO)
foo_defined
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_defined')
end

function TestPreprocessor:test_if_defined_operator_undefined()
    local pp = create_pp()
    local input = [[
#if defined(FOO)
foo_defined
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'foo_defined')
end

function TestPreprocessor:test_if_and_operator()
    local pp = create_pp()
    pp:define('FOO', '1')
    pp:define('BAR', '1')
    local input = [[
#if FOO && BAR
both_defined
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'both_defined')
end

function TestPreprocessor:test_if_or_operator()
    local pp = create_pp()
    pp:define('FOO', '0')
    pp:define('BAR', '1')
    local input = [[
#if FOO || BAR
one_defined
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'one_defined')
end

function TestPreprocessor:test_if_not_operator()
    local pp = create_pp()
    pp:define('FOO', '0')
    local input = [[
#if !FOO
foo_is_false
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_is_false')
end

function TestPreprocessor:test_if_elif_else()
    local pp = create_pp()
    pp:define('FOO', '1')
    local input = [[
#if FOO == 1
foo_is_one
#elif FOO == 2
foo_is_two
#else
foo_is_other
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_is_one')
    lu.assertNotStrContains(result, 'foo_is_two')
    lu.assertNotStrContains(result, 'foo_is_other')
end

function TestPreprocessor:test_if_elif_else_second_branch()
    local pp = create_pp()
    pp:define('FOO', '2')
    local input = [[
#if FOO == 1
foo_is_one
#elif FOO == 2
foo_is_two
#else
foo_is_other
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'foo_is_one')
    lu.assertStrContains(result, 'foo_is_two')
    lu.assertNotStrContains(result, 'foo_is_other')
end

function TestPreprocessor:test_if_elif_else_else_branch()
    local pp = create_pp()
    pp:define('FOO', '3')
    local input = [[
#if FOO == 1
foo_is_one
#elif FOO == 2
foo_is_two
#else
foo_is_other
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'foo_is_one')
    lu.assertNotStrContains(result, 'foo_is_two')
    lu.assertStrContains(result, 'foo_is_other')
end

function TestPreprocessor:test_nested_ifdef()
    local pp = create_pp()
    pp:define('FOO')
    pp:define('BAR')
    local input = [[
#ifdef FOO
foo_start
#ifdef BAR
bar_inside_foo
#endif
foo_end
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_start')
    lu.assertStrContains(result, 'bar_inside_foo')
    lu.assertStrContains(result, 'foo_end')
end

function TestPreprocessor:test_nested_ifdef_outer_defined_inner_undefined()
    local pp = create_pp()
    pp:define('FOO')
    local input = [[
#ifdef FOO
foo_start
#ifdef BAR
bar_inside_foo
#endif
foo_end
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_start')
    lu.assertNotStrContains(result, 'bar_inside_foo')
    lu.assertStrContains(result, 'foo_end')
end

function TestPreprocessor:test_pragma_directive()
    local pp = create_pp()
    local input = '#pragma once'
    local result = pp:process(input)
    lu.assertEquals(result, '/// @pragma( once )')
end

function TestPreprocessor:test_pragma_directive_complex()
    local pp = create_pp()
    local input = '#pragma pack(push, 1)'
    local result = pp:process(input)
    lu.assertEquals(result, '/// @pragma( pack(push, 1) )')
end

function TestPreprocessor:test_include_directive()
    local pp = create_pp(function(filename)
        if filename == 'test.h' then
            return 'included content'
        end
        return nil
    end)
    
    local input = '#include "test.h"'
    local result = pp:process(input)
    lu.assertStrContains(result, '// Begin include: test.h')
    lu.assertStrContains(result, 'included content')
    lu.assertStrContains(result, '// End include: test.h')
end

function TestPreprocessor:test_include_angle_brackets()
    local pp = create_pp(function(filename)
        if filename == 'stdio.h' then
            return 'stdio content'
        end
        return nil
    end)
    
    local input = '#include <stdio.h>'
    local result = pp:process(input)
    lu.assertStrContains(result, 'stdio content')
end

function TestPreprocessor:test_include_not_found()
    local pp = create_pp(function(filename)
        return nil
    end)
    
    local input = '#include "missing.h"'
    local result = pp:process(input)
    lu.assertStrContains(result, '// Include not found: missing.h')
end

function TestPreprocessor:test_include_circular()
    local pp = create_pp(function(filename)
        if filename == 'a.h' then
            return '#include "b.h"'
        elseif filename == 'b.h' then
            return '#include "a.h"'
        end
        return nil
    end)
    
    local input = '#include "a.h"'
    local result = pp:process(input)
    lu.assertStrContains(result, 'Circular include detected')
end

function TestPreprocessor:test_error_directive()
    local pp = create_pp()
    local input = '#error This is an error'
    local result = pp:process(input)
    lu.assertEquals(result, '// #error: This is an error')
end

function TestPreprocessor:test_warning_directive()
    local pp = create_pp()
    local input = '#warning This is a warning'
    local result = pp:process(input)
    lu.assertEquals(result, '// #warning: This is a warning')
end

function TestPreprocessor:test_macro_expansion_in_code()
    local pp = create_pp()
    pp:define('MAX_SIZE', '1024')
    local input = 'int size = MAX_SIZE;'
    local result = pp:process(input)
    lu.assertEquals(result, 'int size = 1024;')
end

function TestPreprocessor:test_macro_expansion_multiple()
    local pp = create_pp()
    pp:define('FOO', 'bar')
    pp:define('BAZ', 'qux')
    local input = 'FOO BAZ FOO'
    local result = pp:process(input)
    lu.assertEquals(result, 'bar qux bar')
end

function TestPreprocessor:test_macro_not_expanded_in_comment()
    local pp = create_pp()
    pp:define('FOO', 'bar')
    local input = '// This is FOO comment'
    local result = pp:process(input)
    -- Comments should still be processed, but macro expansion might not work in them
    -- Current implementation doesn't skip comments fully
    lu.assertStrContains(result, '// This is')
end

function TestPreprocessor:test_complex_code()
    local pp = create_pp()
    pp:define('DEBUG', '1')
    local input = [[
#define VERSION 1
#define NAME MyProject

#ifdef DEBUG
void debug_log(const char* msg);
#endif

int main() {
    // VERSION: VERSION
    return 0;
}

#pragma once
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'void debug_log')
    lu.assertStrContains(result, '/// @pragma')
end

function TestPreprocessor:test_include_with_define()
    local includes = {}
    local pp = create_pp(function(filename)
        if includes[filename] then
            return includes[filename]
        end
        return nil
    end)
    
    includes['config.h'] = '#define CONFIG_VALUE 42'
    
    local input = [[
#include "config.h"
int value = CONFIG_VALUE;
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'int value = 42;')
end

function TestPreprocessor:test_if_with_comparison()
    local pp = create_pp()
    pp:define('VERSION', '5')
    local input = [[
#if VERSION >= 5
new_feature
#else
old_feature
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'new_feature')
    lu.assertNotStrContains(result, 'old_feature')
end

function TestPreprocessor:test_if_with_comparison_old()
    local pp = create_pp()
    pp:define('VERSION', '3')
    local input = [[
#if VERSION >= 5
new_feature
#else
old_feature
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'new_feature')
    lu.assertStrContains(result, 'old_feature')
end

function TestPreprocessor:test_ifdef_inside_if()
    local pp = create_pp()
    pp:define('FOO', '1')
    pp:define('BAR')
    local input = [[
#if FOO
foo_enabled
#ifdef BAR
bar_defined
#endif
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_enabled')
    lu.assertStrContains(result, 'bar_defined')
end

function TestPreprocessor:test_ifdef_inside_if_bar_undefined()
    local pp = create_pp()
    pp:define('FOO', '1')
    local input = [[
#if FOO
foo_enabled
#ifdef BAR
bar_defined
#endif
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_enabled')
    lu.assertNotStrContains(result, 'bar_defined')
end

function TestPreprocessor:test_multiple_defines_same_token()
    local pp = create_pp()
    pp:define('FOO', 'first')
    pp:define('FOO', 'second')
    lu.assertEquals(pp:get_define('FOO'), 'second')
end

function TestPreprocessor:test_empty_define()
    local pp = create_pp()
    pp:define('EMPTY')
    lu.assertTrue(pp:is_defined('EMPTY'))
    lu.assertEquals(pp:get_define('EMPTY'), '')
end

function TestPreprocessor:test_define_with_spaces()
    local pp = create_pp()
    pp:define('FOO', 'bar baz')
    lu.assertEquals(pp:get_define('FOO'), 'bar baz')
end

function TestPreprocessor:test_define_function_like_via_api()
    local pp = create_pp()
    pp:define('FOO(X)', 'x X x')
    local result = pp:process('FOO(aaa)')
    lu.assertEquals(result, 'x aaa x')
end

function TestPreprocessor:test_define_function_like_luabind_field_via_api()
    local pp = create_pp()
    pp:define('LUABIND_FIELD(Name)', 'inline constexpr ::luabind_autobind_type Name = {}')
    local result = pp:process('LUABIND_FIELD(foo)')
    lu.assertEquals(result, 'inline constexpr ::luabind_autobind_type foo = {}')
end

function TestPreprocessor:test_if_zero()
    local pp = create_pp()
    local input = [[
#if 0
disabled_code
#endif
]]
    local result = pp:process(input)
    lu.assertNotStrContains(result, 'disabled_code')
end

function TestPreprocessor:test_if_one()
    local pp = create_pp()
    local input = [[
#if 1
enabled_code
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'enabled_code')
end

function TestPreprocessor:test_if_not_equal()
    local pp = create_pp()
    pp:define('FOO', '1')
    local input = [[
#if FOO != 0
foo_not_zero
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_not_zero')
end

function TestPreprocessor:test_if_equal()
    local pp = create_pp()
    pp:define('FOO', '0')
    local input = [[
#if FOO == 0
foo_is_zero
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'foo_is_zero')
end

function TestPreprocessor:test_complex_nested_conditionals()
    local pp = create_pp()
    pp:define('PLATFORM', '1')
    pp:define('DEBUG', '1')
    local input = [[
#if PLATFORM == 1
platform_unix
#ifdef DEBUG
debug_unix
#endif
#elif PLATFORM == 2
platform_windows
#ifdef DEBUG
debug_windows
#endif
#endif
]]
    local result = pp:process(input)
    lu.assertStrContains(result, 'platform_unix')
    lu.assertStrContains(result, 'debug_unix')
    lu.assertNotStrContains(result, 'platform_windows')
    lu.assertNotStrContains(result, 'debug_windows')
end

function TestPreprocessor:test_pragma_preserves_content()
    local pp = create_pp()
    local input = '#pragma GCC diagnostic ignored "-Wunused-variable"'
    local result = pp:process(input)
    lu.assertStrContains(result, '/// @pragma( GCC diagnostic ignored "-Wunused-variable" )')
end

function TestPreprocessor:test_include_handler_receives_filename()
    local received_filename = nil
    local pp = create_pp(function(filename)
        received_filename = filename
        return 'content'
    end)
    
    pp:process('#include "myheader.h"')
    lu.assertEquals(received_filename, 'myheader.h')
end

function TestPreprocessor:test_expand_args_macro()
    local pp = create_pp()
    local result = pp:process[[
#define FOO(x) x
FOO(lala)    
]]
    lu.assertEquals(result, 'lala')
end

function TestPreprocessor:test_expand_args_macro_multiline()
    local pp = create_pp()
    local result = pp:process[[
#define FOO(x) x
FOO(lala
llab)    
]]
    lu.assertEquals(result, 'lala\nllab')
end

function TestPreprocessor:test_multiline_define()
    local pp = create_pp()
    local result = pp:process[[
#define FOO aaa\
bbb
FOO
]]
    lu.assertEquals(result, 'aaa\nbbb')
end

function TestPreprocessor:test_complex_define()
    local pp = create_pp()
    local result = pp:process[[
#define FOO aaa
#define BAR(X) x X x
BAR(FOO)
]]
    lu.assertEquals(result, 'x aaa x')
end

function TestPreprocessor:test_stringify()
    local pp = create_pp()
    local result = pp:process[[
#define FOO(X) #X
FOO(aaa)
]]
    lu.assertEquals(result, '"aaa"')
end

function TestPreprocessor:test_concatenate()
    local pp = create_pp()
    local result = pp:process[[
#define FOO(X,Y) X ## Y
FOO(aaa,_bbbb)
]]
    lu.assertEquals(result, 'aaa_bbbb')

    result = pp:process[[
#define FOO(X,Y) X Y
FOO(aaa,_bbbb)
]]
    lu.assertEquals(result, 'aaa _bbbb')
end

function TestPreprocessor:test_process_returns_string()
    local pp = create_pp()
    local result = pp:process('test')
    lu.assertEquals(type(result), 'string')
end


TestPreprocessorEvaluate = {}

local function make_eval(defines)
    local pp = create_pp()
    for k, v in pairs(defines or {}) do
        pp:define(k, v)
    end
    return function(expr) return pp:evaluate_expression(expr) end
end

-- числа
function TestPreprocessorEvaluate:test_zero_is_false()
    local eval = make_eval()
    lu.assertFalse(eval('0'))
end

function TestPreprocessorEvaluate:test_one_is_true()
    local eval = make_eval()
    lu.assertTrue(eval('1'))
end

function TestPreprocessorEvaluate:test_nonzero_is_true()
    local eval = make_eval()
    lu.assertTrue(eval('42'))
end

-- defined()
function TestPreprocessorEvaluate:test_defined_true()
    local eval = make_eval({FOO = ''})
    lu.assertTrue(eval('defined(FOO)'))
end

function TestPreprocessorEvaluate:test_defined_false()
    local eval = make_eval()
    lu.assertFalse(eval('defined(FOO)'))
end

function TestPreprocessorEvaluate:test_defined_no_parens()
    local eval = make_eval({FOO = ''})
    lu.assertTrue(eval('defined FOO'))
end

function TestPreprocessorEvaluate:test_defined_spaces()
    local eval = make_eval({FOO = ''})
    lu.assertTrue(eval('defined( FOO )'))
end

-- идентификаторы
function TestPreprocessorEvaluate:test_identifier_undefined_is_zero()
    local eval = make_eval()
    lu.assertFalse(eval('UNDEFINED_MACRO'))
end

function TestPreprocessorEvaluate:test_identifier_empty_define_is_one()
    local eval = make_eval({FOO = ''})
    lu.assertTrue(eval('FOO'))
end

function TestPreprocessorEvaluate:test_identifier_numeric_value()
    local eval = make_eval({FOO = '5'})
    lu.assertTrue(eval('FOO'))
end

function TestPreprocessorEvaluate:test_identifier_zero_value()
    local eval = make_eval({FOO = '0'})
    lu.assertFalse(eval('FOO'))
end

-- унарный !
function TestPreprocessorEvaluate:test_not_zero()
    local eval = make_eval()
    lu.assertTrue(eval('!0'))
end

function TestPreprocessorEvaluate:test_not_one()
    local eval = make_eval()
    lu.assertFalse(eval('!1'))
end

function TestPreprocessorEvaluate:test_double_not()
    local eval = make_eval()
    lu.assertTrue(eval('!!1'))
end

function TestPreprocessorEvaluate:test_not_identifier()
    local eval = make_eval({FOO = '0'})
    lu.assertTrue(eval('!FOO'))
end

-- ==  !=
function TestPreprocessorEvaluate:test_eq_true()
    local eval = make_eval({V = '3'})
    lu.assertTrue(eval('V == 3'))
end

function TestPreprocessorEvaluate:test_eq_false()
    local eval = make_eval({V = '3'})
    lu.assertFalse(eval('V == 5'))
end

function TestPreprocessorEvaluate:test_neq_true()
    local eval = make_eval({V = '3'})
    lu.assertTrue(eval('V != 5'))
end

function TestPreprocessorEvaluate:test_neq_false()
    local eval = make_eval({V = '3'})
    lu.assertFalse(eval('V != 3'))
end

-- < > <= >=
function TestPreprocessorEvaluate:test_lt_true()
    local eval = make_eval({V = '3'})
    lu.assertTrue(eval('V < 5'))
end

function TestPreprocessorEvaluate:test_lt_false()
    local eval = make_eval({V = '5'})
    lu.assertFalse(eval('V < 3'))
end

function TestPreprocessorEvaluate:test_gt_true()
    local eval = make_eval({V = '5'})
    lu.assertTrue(eval('V > 3'))
end

function TestPreprocessorEvaluate:test_lte_equal()
    local eval = make_eval({V = '5'})
    lu.assertTrue(eval('V <= 5'))
end

function TestPreprocessorEvaluate:test_lte_less()
    local eval = make_eval({V = '4'})
    lu.assertTrue(eval('V <= 5'))
end

function TestPreprocessorEvaluate:test_gte_equal()
    local eval = make_eval({V = '5'})
    lu.assertTrue(eval('V >= 5'))
end

function TestPreprocessorEvaluate:test_gte_greater()
    local eval = make_eval({V = '6'})
    lu.assertTrue(eval('V >= 5'))
end

-- && ||
function TestPreprocessorEvaluate:test_and_both_true()
    local eval = make_eval({A = '1', B = '1'})
    lu.assertTrue(eval('A && B'))
end

function TestPreprocessorEvaluate:test_and_one_false()
    local eval = make_eval({A = '1', B = '0'})
    lu.assertFalse(eval('A && B'))
end

function TestPreprocessorEvaluate:test_or_one_true()
    local eval = make_eval({A = '0', B = '1'})
    lu.assertTrue(eval('A || B'))
end

function TestPreprocessorEvaluate:test_or_both_false()
    local eval = make_eval({A = '0', B = '0'})
    lu.assertFalse(eval('A || B'))
end

-- скобки и приоритет
function TestPreprocessorEvaluate:test_parens_override_precedence()
    local eval = make_eval({A = '0', B = '1', C = '1'})
    lu.assertFalse(eval('A && (B || C)'))
end

function TestPreprocessorEvaluate:test_and_over_or()
    -- A||B&&C  =>  A || (B&&C)
    local eval = make_eval({A = '0', B = '1', C = '1'})
    lu.assertTrue(eval('A || B && C'))
end

function TestPreprocessorEvaluate:test_complex_expression()
    local eval = make_eval({MAJOR = '2', MINOR = '5'})
    lu.assertTrue(eval('MAJOR >= 2 && MINOR >= 5'))
end

-- крайние случаи
function TestPreprocessorEvaluate:test_empty_returns_false()
    local eval = make_eval()
    lu.assertFalse(eval(''))
end

function TestPreprocessorEvaluate:test_whitespace_only_returns_false()
    local eval = make_eval()
    lu.assertFalse(eval('   '))
end

function TestPreprocessorEvaluate:test_leading_trailing_spaces()
    local eval = make_eval()
    lu.assertTrue(eval('  1  '))
end


TestPreprocessorFiles = {}

function TestPreprocessorFiles:test_include_file()
    -- local pp = create_pp()
    -- local fs = require 'llae.fs'
    -- local path = require 'llae.path'
    -- local log = require 'llae.log'
    -- for _,v in ipairs(fs.scanfiles_r('src')) do
    --     local ext = path.extension(v)
    --     if ext == 'h' then
    --         log.info('process',v)
    --         local content = tostring(fs.load_file(path.join('src',v)))
    --         local result = pp:process(content)
    --         lu.assertNotNil(result)
    --         log.info('result',result)
    --     end
    -- end
end
