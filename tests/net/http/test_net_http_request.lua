local lu = require('luaunit')
local request = require 'net.http.request'
local uv = require('uv')
local async = require 'llae.async'

--- Run fn in a child coroutine and wait for its completion. Connect attempts
--- may suspend, so they cannot be executed directly from a luaunit test.
local function run_async(fn)
  local finished = false
  local results = {}
  async.run(function()
    local ok, a, b = xpcall(fn, debug.traceback)
    results[1] = ok
    results[2] = a
    results[3] = b
    finished = true
  end)
  while not finished do
    uv.pause(10)
  end
  if not results[1] then
    error(results[2])
  end
  return results[2], results[3]
end

-- Mock for connection to simulate timeouts
local MockConnection = {}
function MockConnection.new(request_obj)
  local self = {}
  self.data = ''
  self.connected = false
  self.closed = false
  self.request = request_obj
  
  function self:connect()
    self.connected = true
    -- Just return true to simulate successful connection
    return true
  end

  function self:write(data)
    if type(data) == 'table' then
      for _,v in ipairs(data) do
        self.data = self.data .. v
      end
    else
      self.data = self.data .. tostring(data)
    end
    -- Return success for write
    return true
  end

  function self:read()
    -- Simulate timeout during read by triggering the timeout callback
    -- and then returning an error
    self.request:_on_timeout()
    return nil, "read error"
  end
  
  function self:close()
    self.closed = true
  end

  function self:check_write(data)
    lu.assertEquals(self.data:gsub('\r\n','\n'),data:gsub('\r\n','\n'))

  end
  
  return self
end

-- Mock for connection that times out during connect
local MockTimeoutConnection = {}
function MockTimeoutConnection.new(request_obj)
  local self = {}
  self.connected = false
  self.closed = false
  self.request = request_obj
  
  function self:connect()
    -- Simulate timeout during connect
    self.request:_on_timeout()
    return nil, "connect error"
  end
  
  function self:close()
    self.closed = true
  end
  
  return self
end

-- Mock for parser
local MockParser = {}
function MockParser.new()
  local self = {}
  
  function self:load(connection)
    return connection:read()
  end
  
  return self
end

-- Test cases
TestHttpRequest = {}

function TestHttpRequest:setUp()
  self.original_create_connection = request._create_connection
  self.original_parser = request.parser
  self.original_resolve = request.resolve
  request.parser = MockParser
end

function TestHttpRequest:tearDown()
  request._create_connection = self.original_create_connection
  request.parser = self.original_parser
  request.resolve = self.original_resolve
end

function TestHttpRequest:test_request_timeout_on_connect()
  local req
  local mock_connection
  request._create_connection = function()
    mock_connection = MockTimeoutConnection.new(req)
    return mock_connection
  end
  
  -- Create request with 1 second timeout
  req = request.new{
    url = "http://example.com",
    timeout = 1
  }
  
  -- Override resolve at instance level, not class level
  function req.resolve(self)
    self._ip_list = {{addr = "127.0.0.1", socktype = "tcp"}}
    return true
  end
  
  -- Execute request and check results
  local response, err = req:exec()
  lu.assertNil(response)
  lu.assertEquals(err, "timeout")
  
  -- Verify that the actual connection was closed
  lu.assertTrue(mock_connection.closed)
end

function TestHttpRequest:test_request_timeout_on_read()
  local req
  local mock_connection
  request._create_connection = function()
    mock_connection = MockConnection.new(req)
    return mock_connection
  end
  
  -- Create request with 1 second timeout
  req = request.new{
    url = "http://example.com",
    timeout = 1
  }
  
  -- Override resolve at instance level, not class level
  function req.resolve(self)
    self._ip_list = {{addr = "127.0.0.1", socktype = "tcp"}}
    return true
  end
  
  -- Execute request and check results
  local response, err = req:exec()
  lu.assertNil(response)
  lu.assertEquals(err, "timeout")
  
  -- Verify that the actual connection was closed
  lu.assertTrue(mock_connection.closed)
end

function TestHttpRequest:test_request_no_timeout()
  -- Create request without timeout
  local req = request.new{
    url = "http://example.com"
  }
  
  -- Override resolve at instance level, not class level
  function req.resolve(self)
    return nil, "dns error"
  end
  
  local response, err = req:exec()
  lu.assertNil(response)
  lu.assertEquals(err, "dns error")
end

function TestHttpRequest:test_connect_tries_next_address_after_error()
  local attempts = {}
  local created = {}
  request._create_connection = function()
    local conn = { closed = false }
    function conn:connect(host, port)
      table.insert(attempts, host)
      if host == '127.0.0.1' then
        return true
      end
      return nil, 'connection refused'
    end
    function conn:close()
      conn.closed = true
    end
    table.insert(created, conn)
    return conn
  end

  local req = request.new{url = "http://example.com", timeout = 1}
  req._ip_list = {
    {addr = '10.0.0.1', socktype = 'tcp'},
    {addr = '127.0.0.1', socktype = 'tcp'},
  }

  local res, err = req:_connect(80)
  lu.assertTrue(res)
  lu.assertNil(err)
  lu.assertEquals(attempts, {'10.0.0.1', '127.0.0.1'})
  lu.assertTrue(created[1].closed)
  lu.assertFalse(created[2].closed)
end

function TestHttpRequest:test_connect_attempt_timeout_tries_next_address()
  local attempts = {}
  local created = {}
  request._create_connection = function()
    local conn = { closed = false }
    local pending
    function conn:connect(host, port)
      table.insert(attempts, host)
      if host == '127.0.0.1' then
        return true
      end
      -- Emulate a blackholed address: the call suspends until close()
      -- cancels it, exactly like a real uv connect.
      pending = coroutine.running()
      coroutine.yield()
      return nil, 'connect canceled'
    end
    function conn:close()
      conn.closed = true
      if pending then
        local co = pending
        pending = nil
        async.resume(co)
      end
    end
    table.insert(created, conn)
    return conn
  end

  -- The request-wide timeout must not take over the per-attempt timeout.
  local req = request.new{url = "http://example.com", timeout = 5, connect_timeout = 0.05}
  req._ip_list = {
    {addr = '10.0.0.1', socktype = 'tcp'},
    {addr = '127.0.0.1', socktype = 'tcp'},
  }

  local res, err = run_async(function()
    return req:_connect(80)
  end)
  lu.assertTrue(res)
  lu.assertNil(err)
  lu.assertEquals(attempts, {'10.0.0.1', '127.0.0.1'})
  lu.assertTrue(created[1].closed)
  lu.assertFalse(created[2].closed)
end

function TestHttpRequest:test_connect_returns_timeout_when_all_attempts_timeout()
  local attempts = {}
  request._create_connection = function()
    local conn = { closed = false }
    local pending
    function conn:connect(host, port)
      table.insert(attempts, host)
      pending = coroutine.running()
      coroutine.yield()
      return nil, 'connect canceled'
    end
    function conn:close()
      conn.closed = true
      if pending then
        local co = pending
        pending = nil
        async.resume(co)
      end
    end
    return conn
  end

  local req = request.new{url = "http://example.com", connect_timeout = 0.05}
  req._ip_list = {
    {addr = '10.0.0.1', socktype = 'tcp'},
    {addr = '10.0.0.2', socktype = 'tcp'},
  }

  local res, err = run_async(function()
    return req:_connect(80)
  end)
  lu.assertNil(res)
  lu.assertEquals(err, 'timeout')
  lu.assertEquals(attempts, {'10.0.0.1', '10.0.0.2'})
end

function TestHttpRequest:test_request_headers()
  local req
  local mock_connection
  request._create_connection = function()
    mock_connection = MockConnection.new(req)
    return mock_connection
  end
  
  -- Create request with 1 second timeout
  req = request.new{
    url = "http://example.com",
    headers = {
      ['X-Test'] = 'test'
    },
    timeout = 1
  }
  
  -- Override resolve at instance level, not class level
  function req.resolve(self)
    self._ip_list = {{addr = "127.0.0.1", socktype = "tcp"}}
    return true
  end
  
  -- Execute request and check results
  local response, err = req:exec()
  lu.assertNil(response)
  lu.assertEquals(err, "timeout")
  
  -- Verify that the actual connection was closed
  lu.assertTrue(mock_connection.closed)

  mock_connection:check_write([[
GET / HTTP/1.1
Host: example.com
Accept-Encoding: deflate, gzip
X-Test: test
Connection: close

]])
end

function TestHttpRequest:test_request_skip_default_headers()
  local req
  local mock_connection
  request._create_connection = function()
    mock_connection = MockConnection.new(req)
    return mock_connection
  end

  req = request.new{
    url = "http://example.com",
    method = "POST",
    body = "abc",
    headers = {
      ['X-Test'] = 'test'
    },
    skip_default_headers = {
      Connection = true,
      HOST = true,
      ['accept-encoding'] = true,
      ['content-length'] = true
    },
    timeout = 1
  }

  -- Override resolve at instance level, not class level
  function req.resolve(self)
    self._ip_list = {{addr = "127.0.0.1", socktype = "tcp"}}
    return true
  end

  local response, err = req:exec()
  lu.assertNil(response)
  lu.assertEquals(err, "timeout")

  mock_connection:check_write([[
POST / HTTP/1.1
X-Test: test

abc]])
end