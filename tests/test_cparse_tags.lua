local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local tags = require('cparse.tags')
package.path = store_path

-- ============================================================
TestTagsParse = {}
-- ============================================================

function TestTagsParse:test_bare_tag()
  local t = tags.parse('@bind')
  lu.assertEquals(#t:get('bind'), 1)
  lu.assertEquals(t:first('bind').tag, 'bind')
  lu.assertEquals(t:first('bind').value, {})
end

function TestTagsParse:test_empty_parens()
  local t = tags.parse('@bind()')
  lu.assertEquals(#t:get('bind'), 1)
  lu.assertEquals(t:first('bind').value, {})
end

function TestTagsParse:test_positional_args()
  local t = tags.parse('@method(name,static)')
  local entry = t:first('method')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.value[1], 'name')
  lu.assertEquals(entry.value[2], 'static')
end

function TestTagsParse:test_named_args()
  local t = tags.parse('@bind(name=foo,type=int)')
  local entry = t:first('bind')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.value['name'], 'foo')
  lu.assertEquals(entry.value['type'], 'int')
end

function TestTagsParse:test_single_named_arg()
  local t = tags.parse('@bind(name=MyClass)')
  lu.assertEquals(t:first('bind').value['name'], 'MyClass')
end

function TestTagsParse:test_arg_values_trimmed()
  local t = tags.parse('@bind( name = foo , type = int )')
  local entry = t:first('bind')
  lu.assertEquals(entry.value['name'], 'foo')
  lu.assertEquals(entry.value['type'], 'int')
end

function TestTagsParse:test_multiple_tags()
  local t = tags.parse('@bind @method')
  lu.assertTrue(t:has('bind'))
  lu.assertTrue(t:has('method'))
end

function TestTagsParse:test_separator_newline()
  local t = tags.parse('@bind\n@method')
  lu.assertTrue(t:has('bind'))
  lu.assertTrue(t:has('method'))
end

function TestTagsParse:test_separator_comma()
  local t = tags.parse('@bind,@method')
  lu.assertTrue(t:has('bind'))
  lu.assertTrue(t:has('method'))
end

function TestTagsParse:test_separator_star()
  local t = tags.parse('@bind * @method')
  lu.assertTrue(t:has('bind'))
  lu.assertTrue(t:has('method'))
end

function TestTagsParse:test_mixed_separators()
  local t = tags.parse(' * @bind(name=foo)\n * @method(static)\n * @deprecated')
  lu.assertEquals(t:first('bind').value['name'], 'foo')
  lu.assertEquals(t:first('method').value[1], 'static')
  lu.assertTrue(t:has('deprecated'))
end

function TestTagsParse:test_no_tags()
  local t = tags.parse('just some plain text, no annotations here')
  lu.assertEquals(t:get('anything'), {})
  lu.assertFalse(t:has('anything'))
end

function TestTagsParse:test_empty_string()
  local t = tags.parse('')
  lu.assertFalse(t:has('bind'))
end

function TestTagsParse:test_at_sign_not_followed_by_ident()
  -- bare '@' or '@123' should not produce a tag
  local t = tags.parse('@ @123 @_valid')
  lu.assertFalse(t:has(''))
  lu.assertFalse(t:has('123'))
  lu.assertTrue(t:has('_valid'))
end

function TestTagsParse:test_tag_name_with_underscores_and_digits()
  local t = tags.parse('@my_tag_2(val)')
  lu.assertNotNil(t:first('my_tag_2'))
  lu.assertEquals(t:first('my_tag_2').value[1], 'val')
end

-- ============================================================
TestTagsGet = {}
-- ============================================================

function TestTagsGet:test_get_returns_all()
  local t = tags.parse('@bind(a) @bind(b) @bind(c)')
  lu.assertEquals(#t:get('bind'), 3)
end

function TestTagsGet:test_get_returns_empty_for_missing()
  local t = tags.parse('@bind')
  lu.assertEquals(t:get('missing'), {})
end

function TestTagsGet:test_first_returns_nil_for_missing()
  local t = tags.parse('@bind')
  lu.assertNil(t:first('missing'))
end

function TestTagsGet:test_first_returns_first_entry()
  local t = tags.parse('@bind(a=1) @bind(a=2)')
  local entry = t:first('bind')
  lu.assertEquals(entry.value['a'], '1')
end

function TestTagsGet:test_has_true()
  local t = tags.parse('@bind')
  lu.assertTrue(t:has('bind'))
end

function TestTagsGet:test_has_false()
  local t = tags.parse('@bind')
  lu.assertFalse(t:has('method'))
end

-- ============================================================
TestTagsForeach = {}
-- ============================================================

function TestTagsForeach:test_foreach_iterates_matched()
  local t = tags.parse('@bind(x=1) @other @bind(x=2)')
  local xs = {}
  for _, entry in t:foreach('bind') do
    table.insert(xs, entry.value['x'])
  end
  lu.assertEquals(xs, { '1', '2' })
end

function TestTagsForeach:test_foreach_empty_when_missing()
  local t = tags.parse('@bind')
  local count = 0
  for _ in t:foreach('missing') do
    count = count + 1
  end
  lu.assertEquals(count, 0)
end

-- ============================================================
TestTagsCheckUnique = {}
-- ============================================================

function TestTagsCheckUnique:test_unique_present()
  local t = tags.parse('@bind(name=foo)')
  local entry = t:check_unique('bind')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.value['name'], 'foo')
end

function TestTagsCheckUnique:test_unique_absent()
  local t = tags.parse('@bind')
  local entry = t:check_unique('missing')
  lu.assertNil(entry)
end

function TestTagsCheckUnique:test_unique_error_on_duplicates()
  local t = tags.parse('@bind @bind')
  lu.assertError(function() t:check_unique('bind') end)
end

-- ============================================================
TestTagsCollect = {}
-- ============================================================

function TestTagsCollect:test_collect_single()
  local t = tags.parse('@bind(name=foo,type=int)')
  local v = t:collect('bind')
  lu.assertEquals(v['name'], 'foo')
  lu.assertEquals(v['type'], 'int')
end

function TestTagsCollect:test_collect_merges_multiple()
  local t = tags.parse('@bind(name=foo) @bind(type=int)')
  local v = t:collect('bind')
  lu.assertEquals(v['name'], 'foo')
  lu.assertEquals(v['type'], 'int')
end

function TestTagsCollect:test_collect_later_overwrites()
  local t = tags.parse('@bind(name=foo) @bind(name=bar)')
  local v = t:collect('bind')
  lu.assertEquals(v['name'], 'bar')
end

function TestTagsCollect:test_collect_empty_when_missing()
  local t = tags.parse('@bind')
  local v = t:collect('missing')
  lu.assertNil(v)
end

function TestTagsCollect:test_collect_positional()
  local t = tags.parse('@flags(a,b) @flags(c)')
  local v = t:collect('flags')
  -- positional keys are 1,2 from first entry then 1 from second overwrites
  lu.assertEquals(v[1], 'c')
  lu.assertEquals(v[2], 'b')
end

-- ============================================================
TestTagsComment = {}
-- ============================================================

function TestTagsComment:test_bare_tag_with_comment()
  local t = tags.parse('@bind Some text here')
  local entry = t:first('bind')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.comment, 'Some text here')
end

function TestTagsComment:test_tag_with_args_and_comment()
  local t = tags.parse('@lparam(name,string) The parameter description')
  local entry = t:first('lparam')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.value[1], 'name')
  lu.assertEquals(entry.value[2], 'string')
  lu.assertEquals(entry.comment, 'The parameter description')
end

function TestTagsComment:test_tag_no_comment()
  local t = tags.parse('@bind')
  local entry = t:first('bind')
  lu.assertNotNil(entry)
  lu.assertNil(entry.comment)
end

function TestTagsComment:test_tag_with_args_no_comment()
  local t = tags.parse('@bind(name=foo)')
  local entry = t:first('bind')
  lu.assertNotNil(entry)
  lu.assertNil(entry.comment)
end

function TestTagsComment:test_comment_trimmed()
  local t = tags.parse('@bind   trimmed   ')
  local entry = t:first('bind')
  lu.assertEquals(entry.comment, 'trimmed')
end

function TestTagsComment:test_comment_trimmed_after_args()
  local t = tags.parse('@bind(x)   trimmed   ')
  local entry = t:first('bind')
  lu.assertEquals(entry.comment, 'trimmed')
end

function TestTagsComment:test_multiline_each_tag_gets_own_comment()
  local t = tags.parse('@lparam(a,integer) First param\n@lparam(b,string) Second param')
  local entries = t:get('lparam')
  lu.assertEquals(#entries, 2)
  lu.assertEquals(entries[1].value[1], 'a')
  lu.assertEquals(entries[1].comment, 'First param')
  lu.assertEquals(entries[2].value[1], 'b')
  lu.assertEquals(entries[2].comment, 'Second param')
end

function TestTagsComment:test_clean_lines_not_affected()
  local t = tags.parse('Clean description\n@bind Some comment')
  lu.assertEquals(t:first('bind').comment, 'Some comment')
  local clean = t:get_clean()
  lu.assertNotNil(clean)
  lu.assertEquals(clean[1], 'Clean description')
end

function TestTagsComment:test_comment_with_doc_comment_prefix()
  -- typical C++ doc comment block
  local text = '/// @lparam(data,string|llae.buffer_base) The data to process'
  local t = tags.parse(text)
  local entry = t:first('lparam')
  lu.assertNotNil(entry)
  lu.assertEquals(entry.value[1], 'data')
  lu.assertEquals(entry.comment, 'The data to process')
end

function TestTagsComment:test_multiline_doc_comment_clean_preserved()
  local text = '/// Brief description\n/// @lparam(x,integer) The x value'
  local t = tags.parse(text)
  lu.assertEquals(t:first('lparam').comment, 'The x value')
  local clean = t:get_clean()
  lu.assertNotNil(clean)
  lu.assertEquals(clean[1], 'Brief description')
end

