local lu = require('luaunit')
local json_middleware = require('web.json')

local function new_handler()
  local handler
  local app = {
    register_handler = function(_, fn)
      handler = fn
    end,
  }
  json_middleware.new():use(app)
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

local function make_response()
  local res = {}
  function res:set_header(name, value)
    self.headers = self.headers or {}
    self.headers[name] = value
  end
  function res:finish(value)
    self.finished_value = value
    return value
  end
  return res
end

local function run(content_type, body)
  local handler = new_handler()
  local req = make_request(content_type, body)
  local res = make_response()
  handler(req, res)
  return req, res
end

TestWebJson = {}

function TestWebJson:test_parses_exact_content_type()
  local req = run('application/json', '{"a":1}')
  lu.assertEquals(req.json.a, 1)
  lu.assertNil(req.json_error)
  lu.assertEquals(req.read_calls, 1)
end

function TestWebJson:test_parses_content_type_with_charset()
  local req = run('application/json; charset=utf-8', '{"a":1}')
  lu.assertEquals(req.json.a, 1)
  lu.assertNil(req.json_error)
  lu.assertEquals(req.read_calls, 1)
end

function TestWebJson:test_parses_content_type_with_charset_and_spaces()
  local req = run('application/json ;charset=UTF-8', '{"a":1}')
  lu.assertEquals(req.json.a, 1)
  lu.assertNil(req.json_error)
end

function TestWebJson:test_parses_content_type_case_insensitively()
  local req = run('Application/JSON; charset=UTF-8', '{"a":1}')
  lu.assertEquals(req.json.a, 1)
  lu.assertNil(req.json_error)
end

function TestWebJson:test_ignores_other_parameters()
  local req = run('application/json; charset=utf-8; boundary=something', '{"a":1}')
  lu.assertEquals(req.json.a, 1)
  lu.assertNil(req.json_error)
end

function TestWebJson:test_skips_other_content_types()
  local req = run('text/plain; charset=utf-8', '{"a":1}')
  lu.assertNil(req.json)
  lu.assertNil(req.json_error)
  lu.assertEquals(req.read_calls, 0)
end

function TestWebJson:test_skips_missing_content_type()
  local req = run(nil, '{"a":1}')
  lu.assertNil(req.json)
  lu.assertNil(req.json_error)
  lu.assertEquals(req.read_calls, 0)
end

function TestWebJson:test_sets_json_response()
  local req, res = run('text/plain', '')
  res:json({ ok = true })
  lu.assertEquals(res.headers['Content-Type'], 'application/json')
  lu.assertEquals(res.finished_value, '{"ok":true}')
end
