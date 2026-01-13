local lu = require('luaunit')
local request = require 'net.http.request'
local uv = require('uv')

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