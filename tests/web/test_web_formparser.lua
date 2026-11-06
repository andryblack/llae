local lu = require('luaunit')
local formparser = require('web.formparser')

local function new_handler()
  local handler
  local web = {
    register_handler = function(_, fn)
      handler = fn
    end,
  }
  formparser.new():use(web)
  return handler
end

local function make_request(content_type, body)
  local req = {
    read_calls = 0,
  }
  function req:get_header(name)
    if name == 'Content-Type' then
      return content_type
    end
  end
  function req:read_body()
    self.read_calls = self.read_calls + 1
    return body
  end
  return req
end

local function run(content_type, body)
  local req = make_request(content_type, body)
  new_handler()(req, {})
  return req
end

TestWebFormparser = {}

function TestWebFormparser:test_parses_exact_content_type()
  local req = run('application/x-www-form-urlencoded', 'name=test&value=42')
  lu.assertEquals(req.form.name, 'test')
  lu.assertEquals(req.form.value, '42')
  lu.assertEquals(req.read_calls, 1)
end

function TestWebFormparser:test_parses_content_type_with_charset()
  local req = run('application/x-www-form-urlencoded; charset=utf-8', 'name=test')
  lu.assertEquals(req.form.name, 'test')
  lu.assertEquals(req.read_calls, 1)
end

function TestWebFormparser:test_parses_content_type_with_charset_case_insensitively()
  local req = run('Application/X-WWW-Form-Urlencoded ; charset=UTF-8', 'name=test')
  lu.assertEquals(req.form.name, 'test')
  lu.assertEquals(req.read_calls, 1)
end

function TestWebFormparser:test_skips_other_content_types()
  local req = run('application/json; charset=utf-8', '{"name":"test"}')
  lu.assertNil(req.form)
  lu.assertEquals(req.read_calls, 0)
end

function TestWebFormparser:test_skips_missing_content_type()
  local req = run(nil, 'name=test')
  lu.assertNil(req.form)
  lu.assertEquals(req.read_calls, 0)
end
