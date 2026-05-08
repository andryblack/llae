local lu = require('luaunit')
local class = require('llae.class')
local views = require('web.views')
local template = require('llae.template')

local TestResponse = class(nil, 'test.web.response')

function TestResponse:_init()
  self.status_code = nil
  self.headers = {}
  self.finished_value = nil
end

function TestResponse:status(code)
  self.status_code = code
  return self
end

function TestResponse:set_header(name, value)
  self.headers[name] = value
end

function TestResponse:finish(value)
  self.finished_value = value
  return value
end

local TestableViews = class(views, 'test.web.views')

function TestableViews:_init(root, options)
  TestableViews.baseclass._init(self, root, options)
  self.template_sources = {}
  self.parts_sources = {}
  self._resolved_views = {}
  self.calls = {
    load_template = 0,
    load_parts_source = 0,
    compile_part = 0,
  }
end

function TestableViews:_resolve_view_path(view)
  local fn = '/virtual/' .. self._root .. '/' .. view .. '.' .. self._ext
  self.last_resolved_relative = self._root .. '/' .. view .. '.' .. self._ext
  self._resolved_views[fn] = view
  return fn
end

function TestableViews:_load_template(fn, options)
  self.calls.load_template = self.calls.load_template + 1
  local view = self._resolved_views[fn] or options.name
  local source = self.template_sources[view]
  if source then
    return template.compile(source, options)
  end
  return TestableViews.baseclass._load_template(self, fn, options)
end

function TestableViews:_load_parts_source(fn)
  self.calls.load_parts_source = self.calls.load_parts_source + 1
  local view = self._resolved_views[fn]
  local source = self.parts_sources[view]
  if source then
    return source
  end
  return TestableViews.baseclass._load_parts_source(self, fn)
end

function TestableViews:_compile_part(part, view, part_name)
  self.calls.compile_part = self.calls.compile_part + 1
  return TestableViews.baseclass._compile_part(self, part, view, part_name)
end

TestWebViews = {}

function TestWebViews:test_get_uses_cache_when_nocache_false()
  local v = TestableViews.new('templates', { env = {}, nocache = false })
  v.template_sources.home = 'HOME'

  local t1 = v:get('home')
  local t2 = v:get('home')

  lu.assertEquals(t1, t2)
  lu.assertEquals(t1({}), 'HOME')
  lu.assertEquals(v.last_resolved_relative, 'templates/home.thtml')
  lu.assertEquals(v.calls.load_template, 1)
end

function TestWebViews:test_get_disables_cache_when_nocache_true()
  local v = TestableViews.new('templates', { env = {}, nocache = true })
  v.template_sources.profile = 'Hi <%= name %>'

  local t1 = v:get('profile')
  local t2 = v:get('profile')

  lu.assertNotEquals(t1, t2)
  lu.assertEquals(t1({ name = 'Bob' }), 'Hi Bob')
  lu.assertEquals(t2({ name = 'Bob' }), 'Hi Bob')
  lu.assertEquals(v.calls.load_template, 2)
end

function TestWebViews:test_get_parts_parses_parts_and_caches()
  local v = TestableViews.new('templates', { env = { tag = 'ENV' }, nocache = false })
  v.parts_sources.layout = 'base:<%= tag %>-{header}-head:<%= tag %>-{footer}-foot:<%= tag %>'

  local parts1 = v:get_parts('layout')
  local parts2 = v:get_parts('layout')

  lu.assertEquals(parts1, parts2)
  lu.assertEquals(parts1.base({}), 'base:ENV')
  lu.assertEquals(parts1.header({ tag = 'LOCAL' }), 'head:LOCAL')
  lu.assertEquals(parts1.footer({}), 'foot:ENV')
  lu.assertEquals(v.calls.load_parts_source, 1)
  lu.assertEquals(v.calls.compile_part, 3)
end

function TestWebViews:test_include_merges_context_and_prefers_local_values()
  local v = TestableViews.new('templates', { env = { conflict = 'ENV' }, nocache = false })
  v.template_sources.child = '<%= conflict %>'

  local result = v._funcs.include('child', { conflict = 'LOCAL' })
  lu.assertEquals(result, 'LOCAL')
end

function TestWebViews:test_include_parts_returns_parts_table()
  local v = TestableViews.new('templates', { env = {} })
  local expected = { base = function() return 'X' end }
  v.get_parts = function(_, view)
    lu.assertEquals(view, 'child')
    return expected
  end

  local result = v._funcs.include_parts('child')
  lu.assertEquals(result, expected)
end

function TestWebViews:test_render_success_sets_html_content_type_and_finishes()
  local v = TestableViews.new('templates', { env = {} })
  v.get = function()
    return function(ctx)
      return 'Hello ' .. tostring(ctx.name)
    end
  end

  local resp = TestResponse.new()
  local result = v:_render(resp, 'index', { name = 'Alice' })

  lu.assertEquals(result, 'Hello Alice')
  lu.assertEquals(resp.headers['Content-Type'], 'text/html')
  lu.assertNil(resp.status_code)
  lu.assertEquals(resp.finished_value, 'Hello Alice')
end

function TestWebViews:test_render_error_returns_500_and_localized_rendered_message()
  local v = TestableViews.new('templates', { env = {} })
  v.get = function()
    return function()
      error('boom_localized')
    end
  end

  local resp = TestResponse.new()
  local result = v:_render(resp, 'broken', { locale = 'ru' })

  lu.assertEquals(result, resp.finished_value)
  lu.assertEquals(resp.status_code, 500)
  lu.assertNil(resp.headers['Content-Type'])
  lu.assertEquals(string.sub(resp.finished_value, 1, #'Error rendering template: '), 'Error rendering template: ')
  lu.assertStrContains(resp.finished_value, 'boom_localized')
end

function TestWebViews:test_render_error_inside_template_code_block_is_localized()
  local v = TestableViews.new('templates', { env = {} })
  v.template_sources.broken_tpl = [[
Test
<% error('test-error') %>
]]

  local resp = TestResponse.new()
  local result = v:_render(resp, 'broken_tpl', {})

  lu.assertEquals(result, resp.finished_value)
  lu.assertEquals(resp.status_code, 500)
  lu.assertNil(resp.headers['Content-Type'])
  lu.assertEquals(string.sub(resp.finished_value, 1, #'Error rendering template: '), 'Error rendering template: ')
  lu.assertStrContains(resp.finished_value, 'test-error')
  lu.assertStrContains(resp.finished_value, 'broken_tpl')
end

function TestWebViews:test_render_error_inside_include_is_localized()
  local v = TestableViews.new('templates', { env = {} })
  v.template_sources.page = [[
<div>
<%- include('partial') %>
</div>
]]
  v.template_sources.partial = [[
<% error('include-test-error') %>
]]

  local resp = TestResponse.new()
  local result = v:_render(resp, 'page', {})

  lu.assertEquals(result, resp.finished_value)
  lu.assertEquals(resp.status_code, 500)
  lu.assertNil(resp.headers['Content-Type'])
  lu.assertEquals(string.sub(resp.finished_value, 1, #'Error rendering template: '), 'Error rendering template: ')
  lu.assertStrContains(resp.finished_value, 'include-test-error')
  lu.assertStrContains(resp.finished_value, 'partial')
end

function TestWebViews:test_render_error_inside_include_parts_is_localized()
  local v = TestableViews.new('templates', { env = {} })
  v.template_sources.page_parts = [[
<%
  local parts = include_parts('parts_tpl')
%>
<div>
<%- parts.fail() %>
</div>
]]
  v.parts_sources.parts_tpl = [[
ok-part
-{fail}-
<% error('parts-test-error') %>
]]

  local resp = TestResponse.new()
  local result = v:_render(resp, 'page_parts', {})

  lu.assertEquals(result, resp.finished_value)
  lu.assertEquals(resp.status_code, 500)
  lu.assertNil(resp.headers['Content-Type'])
  lu.assertEquals(string.sub(resp.finished_value, 1, #'Error rendering template: '), 'Error rendering template: ')
  lu.assertStrContains(resp.finished_value, 'parts-test-error')
  lu.assertStrContains(resp.finished_value, 'parts_tpl/fail')
end

return TestWebViews

