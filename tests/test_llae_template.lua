local lu = require 'luaunit'
local template = require 'llae.template'
local fs = require 'llae.fs'
local path = require 'llae.path'

TestTemplate = {}

function TestTemplate:test_escape_function()
  -- Test basic HTML escaping
  lu.assertEquals(template.escape('<tag>'), '&lt;tag&gt;')
  lu.assertEquals(template.escape('"quoted"'), '&quot;quoted&quot;')
  lu.assertEquals(template.escape("'single'"), '&#39;single&#39;')
  lu.assertEquals(template.escape('&amp;'), '&amp;amp;')
  lu.assertEquals(template.escape('/slash'), '&#47;slash')
  lu.assertEquals(template.escape('>more'), '&gt;more')
  
  -- Test combined escaping
  lu.assertEquals(template.escape('<script>alert("xss")</script>'), 
    '&lt;script&gt;alert(&quot;xss&quot;)&lt;&#47;script&gt;')
  
  -- Test non-string types
  lu.assertEquals(template.escape(123), '123')
  lu.assertEquals(template.escape(true), 'true')
  lu.assertEquals(template.escape(nil), 'nil')
end

function TestTemplate:test_plain_text()
  -- Test template with no tags
  local result = template.render('Hello World', {})
  lu.assertEquals(result, 'Hello World')
  
  -- Test empty template
  result = template.render('', {})
  lu.assertEquals(result, '')
  
  -- Test multiline plain text
  result = template.render('Line 1\nLine 2\nLine 3', {})
  lu.assertEquals(result, 'Line 1\nLine 2\nLine 3')
end

function TestTemplate:test_escaped_output()
  -- Test <%= with simple variable
  local result = template.render('<%= name %>', {name = 'John'})
  lu.assertEquals(result, 'John')
  
  -- Test <%= with HTML characters (should be escaped)
  result = template.render('<%= content %>', {content = '<script>alert("xss")</script>'})
  lu.assertEquals(result, '&lt;script&gt;alert(&quot;xss&quot;)&lt;&#47;script&gt;')
  
  -- Test <%= with number
  result = template.render('<%= age %>', {age = 42})
  lu.assertEquals(result, '42')
  
  -- Test <%= with nil
  result = template.render('Hello <%= name %>', {name = nil})
  lu.assertEquals(result, 'Hello nil')
end

function TestTemplate:test_raw_output()
  -- Test <%- with simple variable
  local result = template.render('<%- name %>', {name = 'John'})
  lu.assertEquals(result, 'John')
  
  -- Test <%- with HTML characters (should NOT be escaped)
  result = template.render('<%- html %>', {html = '<b>bold</b>'})
  lu.assertEquals(result, '<b>bold</b>')
  
  -- Test <%- with complex HTML
  result = template.render('<%- content %>', {content = '<div class="test">Hello</div>'})
  lu.assertEquals(result, '<div class="test">Hello</div>')
end

function TestTemplate:test_code_blocks()
  -- Test simple code block
  local result = template.render('<% local x = 5 %><%= x %>', {})
  lu.assertEquals(result, '5')
  
  -- Test loop
  result = template.render('<% for i=1,3 do %><%= i %><% end %>', {})
  lu.assertEquals(result, '123')
  
  -- Test loop with separator
  result = template.render('<% for i=1,3 do %><%= i %><% if i < 3 then %>:<% end %><% end %>', {})
  lu.assertEquals(result, '1:2:3')
  
  -- Test conditional
  result = template.render('<% if flag then %>Yes<% else %>No<% end %>', {flag = true})
  lu.assertEquals(result, 'Yes')
  
  result = template.render('<% if flag then %>Yes<% else %>No<% end %>', {flag = false})
  lu.assertEquals(result, 'No')
end

function TestTemplate:test_mixed_content()
  -- Test combination of plain text, escaped, raw, and code
  local result = template.render(
    'Hello <%= name %>! Your HTML: <%- html %><% for i=1,2 do %> Item <%= i %><% end %>',
    {name = '<John>', html = '<b>bold</b>'}
  )
  lu.assertEquals(result, 'Hello &lt;John&gt;! Your HTML: <b>bold</b> Item 1 Item 2')
  
  -- Test complex template
  result = template.render(
    [[<h1><%= title %></h1>
<ul><% for i=1,3 do %>
  <li><%= i %>. item</li>
<% end %></ul>]],
    {title = 'List'}
  )
  lu.assertStrContains(result, '<h1>List</h1>')
  lu.assertStrContains(result, '<li>1. item</li>')
  lu.assertStrContains(result, '<li>2. item</li>')
  lu.assertStrContains(result, '<li>3. item</li>')
end

function TestTemplate:test_context_variables()
  -- Test accessing nested properties
  local result = template.render('<%= user.name %> is <%= user.age %> years old', {
    user = {name = 'Alice', age = 30}
  })
  lu.assertEquals(result, 'Alice is 30 years old')
  
  -- Test accessing array elements
  result = template.render('<% for i=1,#items do %><%= items[i] %><% end %>', {
    items = {'a', 'b', 'c'}
  })
  lu.assertEquals(result, 'abc')
  
  -- Test multiple variables
  result = template.render('Name: <%= name %>, Age: <%= age %>, Active: <%= active %>', {
    name = 'Bob',
    age = 25,
    active = true
  })
  lu.assertEquals(result, 'Name: Bob, Age: 25, Active: true')
end

function TestTemplate:test_custom_tags()
  -- Test custom open_tag and close_tag
  -- Note: Custom tags must be 2 characters (same length as default '<%' and '%>')
  local options = {
    open_tag = '{$',
    close_tag = '$}'
  }
  
  local result = template.render('{$= name $}', {name = 'John'}, options)
  lu.assertEquals(result, 'John')
  
  -- Test custom tags with raw output (no space after -)
  result = template.render('Hello {$-name$}!', {name = 'World'}, options)
  lu.assertEquals(result, 'Hello World!')
  
  -- Test custom tags with code block
  result = template.render('Hello {$ local x = name; _s(x) $}!', {name = 'World'}, options)
  lu.assertEquals(result, 'Hello World!')
  
  -- Test custom tags with code
  result = template.render('{$ for i=1,3 do $}{$= i $}{$ end $}', {}, options)
  lu.assertEquals(result, '123')
end

function TestTemplate:test_default_tags_similar()
  -- Test similar cases with default tags to compare behavior
  -- Raw output with text after
  result = template.render('Hello <%-name%>!', {name = 'World'})
  lu.assertEquals(result, 'Hello World!')
  
  -- Loop with text after
  result = template.render('<% for i=1,3 do %><%= i %><% end %>!', {})
  lu.assertEquals(result, '123!')
end

function TestTemplate:test_custom_environment()
  -- Test custom functions in environment
  local options = {
    env = {
      upper = function(str)
        return string.upper(str)
      end,
      lower = function(str)
        return string.lower(str)
      end
    }
  }
  
  local result = template.render('<%= upper(name) %>', {name = 'hello'}, options)
  lu.assertEquals(result, 'HELLO')
  
  result = template.render('<%= lower(name) %>', {name = 'WORLD'}, options)
  lu.assertEquals(result, 'world')
  
  -- Test using escape function from environment
  -- Note: When using <%= escape(...) %>, escape is called and then _e escapes again
  -- So we get double escaping. For raw escaped output, use <%- escape(...) %>
  result = template.render('<%= escape(content) %>', {content = '<script>'}, options)
  -- The escape function returns escaped HTML, and _e will escape it again (double escaping)
  lu.assertEquals(result, '&amp;lt;script&amp;gt;')
  
  -- Test using <%- for single escaping
  result = template.render('<%- escape(content) %>', {content = '<script>'}, options)
  lu.assertEquals(result, '&lt;script&gt;')
end

function TestTemplate:test_empty_and_whitespace()
  -- Test empty string in output
  local result = template.render('<%= empty %>', {empty = ''})
  lu.assertEquals(result, '')
  
  -- Test whitespace handling
  result = template.render('  <%= name %>  ', {name = 'Test'})
  lu.assertEquals(result, '  Test  ')
  
  -- Test newlines
  result = template.render('<%= name %>\n<%= name %>', {name = 'X'})
  lu.assertEquals(result, 'X\nX')
end

function TestTemplate:test_compile_function()
  -- Test compile returns function
  local compiled = template.compile('<%= name %>')
  lu.assertIsFunction(compiled)
  
  -- Test compiled function works
  local result = compiled({name = 'Test'})
  lu.assertEquals(result, 'Test')
  
  -- Test compiled function can be called multiple times
  result = compiled({name = 'First'})
  lu.assertEquals(result, 'First')
  
  result = compiled({name = 'Second'})
  lu.assertEquals(result, 'Second')
end

function TestTemplate:test_template_instance()
  -- Note: template.new() is not exported, use compile/render functions instead
  -- This test verifies that we can compile and render separately
  local compiled = template.compile('<%= name %>')
  lu.assertIsFunction(compiled)
  
  local result = compiled({name = 'Instance'})
  lu.assertEquals(result, 'Instance')
  
  -- Test compile with options
  compiled = template.compile('<%= value %>', {name = 'mytemplate'})
  result = compiled({value = 42})
  lu.assertEquals(result, '42')
  
  -- Test compile and render separately
  compiled = template.compile('Hello <%= name %>')
  lu.assertIsFunction(compiled)
  result = compiled({name = 'World'})
  lu.assertEquals(result, 'Hello World')
end

function TestTemplate:test_load_file()
  -- Create a temporary template file
  local test_dir = path.join('build', 'tests', 'tmp')
  if not fs.isdir(test_dir) then
    fs.mkdir_r(test_dir)
  end
  
  local test_file = path.join(test_dir, 'test_template.thtml')
  fs.write_file(test_file, 'Hello <%= name %>')
  
  -- Test loading template from file
  local result = template.render_file(test_file, {name = 'FileTest'})
  lu.assertEquals(result, 'Hello FileTest')
  
  -- Test load function returns compiled template
  local compiled = template.load(test_file)
  lu.assertIsFunction(compiled)
  result = compiled({name = 'LoadTest'})
  lu.assertEquals(result, 'Hello LoadTest')
  
  -- Cleanup
  if fs.isfile(test_file) then
    fs.unlink(test_file)
  end
end

function TestTemplate:test_nested_structures()
  -- Test nested loops
  local result = template.render([[
<% for i=1,2 do %>
  Row <%= i %>:
  <% for j=1,2 do %>
    Col <%= j %>
  <% end %>
<% end %>]], {})
  
  lu.assertStrContains(result, 'Row 1')
  lu.assertStrContains(result, 'Row 2')
  lu.assertStrContains(result, 'Col 1')
  lu.assertStrContains(result, 'Col 2')
  
  -- Test nested conditions
  result = template.render([[
<% if outer then %>
  Outer true
  <% if inner then %>
    Inner true
  <% end %>
<% end %>]], {outer = true, inner = true})
  
  lu.assertStrContains(result, 'Outer true')
  lu.assertStrContains(result, 'Inner true')
end

function TestTemplate:test_edge_cases()
  -- Test empty code block
  local result = template.render('<%do end%>', {})
  lu.assertEquals(result, '')
  
  -- Test code block at start
  result = template.render('<% local x = "test" %><%= x %>', {})
  lu.assertEquals(result, 'test')
  
  -- Test code block at end
  result = template.render('<%= name %><% local x = name %>', {name = 'End'})
  lu.assertEquals(result, 'End')
  
  -- Test consecutive tags
  result = template.render('<%= a %><%= b %><%= c %>', {a = 1, b = 2, c = 3})
  lu.assertEquals(result, '123')
  
  -- Test tags with no content between
  result = template.render('<%= a %><% local x = 1 %><%= b %>', {a = 'A', b = 'B'})
  lu.assertEquals(result, 'AB')
end

function TestTemplate:test_global_functions_access()
  -- Test accessing global functions like math
  local result = template.render('<%= math.max(1, 2, 3) %>', {})
  lu.assertEquals(result, '3')
  
  -- Test accessing string functions
  result = template.render('<%= string.len("hello") %>', {})
  lu.assertEquals(result, '5')
  
  -- Test accessing os functions
  result = template.render('<%= type(os.date) %>', {})
  lu.assertEquals(result, 'function')
end

function TestTemplate:test_multiline_code()
  -- Test multiline code block
  local result = template.render([[
<% 
  local sum = 0
  for i=1,5 do
    sum = sum + i
  end
%><%= sum %>]], {})
  lu.assertEquals(result, '15')
  
  -- Test multiline with conditions
  result = template.render([[
<%
  local items = {'a', 'b', 'c'}
  for i=1,#items do
%>
  Item <%= items[i] %>
<%
  end
%>]], {})
  
  lu.assertStrContains(result, 'Item a')
  lu.assertStrContains(result, 'Item b')
  lu.assertStrContains(result, 'Item c')
end

function TestTemplate:test_debug_mode()
  -- Test debug mode option
  local options = {debug = true}
  local compiled = template.compile('<%= name %>', options)
  lu.assertIsFunction(compiled)
  
  -- In debug mode, compilation errors should provide better messages
  -- This is harder to test without actual errors, but we can verify it doesn't break normal operation
  local result = compiled({name = 'Debug'})
  lu.assertEquals(result, 'Debug')
end

return TestTemplate

