local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'
local request = require 'net.http.request'

-- Integration tests for timeout hangs (esp. HTTPS / SOCKS5 proxy).
-- Several cases intentionally fail until timeout cancellation is fixed.

local next_port = 29100 + (os.time() % 1000)

local function alloc_port()
  next_port = next_port + 1
  return next_port
end

---@class HangTestServer
---@field port integer
---@field _server uv.tcp_server
---@field _clients table
---@field _accept_thread llae.async.thread?
---@field _stopped boolean

local function start_tcp_server(port, on_client)
  local server = uv.tcp_server.new()
  local ok, err = server:bind('127.0.0.1', port)
  if not ok then
    return nil, err
  end
  ---@type HangTestServer
  local state = {
    port = port,
    _server = server,
    _clients = {},
    _stopped = false,
  }
  state._accept_thread = async.run(function()
    while not state._stopped do
      local res, lerr = server:listen(16)
      if not res then
        break
      end
      local client = uv.tcp_connection.new()
      local aok, aerr = server:accept(client)
      if aok then
        table.insert(state._clients, client)
        if on_client then
          async.run(function()
            on_client(client)
          end)
        end
      else
        client:close()
        if aerr then
          break
        end
      end
    end
  end)
  function state:stop()
    self._stopped = true
    for _, c in ipairs(self._clients) do
      pcall(function() c:close() end)
    end
    self._clients = {}
    pcall(function() self._server:stop() end)
  end
  return state
end

--- Accept TCP and never reply (blackhole after accept).
local function start_silent_server(port)
  return start_tcp_server(port, nil)
end

--- Minimal SOCKS5: greet + CONNECT success, then stay silent (no target data).
local function start_socks5_connect_ok_silent(port)
  return start_tcp_server(port, function(client)
    -- method selection
    local buf, err = client:read()
    if not buf then
      return
    end
    local data = tostring(buf)
    if #data < 2 or data:byte(1) ~= 0x05 then
      return
    end
    assert(client:write(string.char(0x05, 0x00)))

    -- CONNECT request
    buf, err = client:read()
    if not buf then
      return
    end
    -- VER REP RSV ATYP BND.ADDR BND.PORT
    local reply = string.char(
      0x05, 0x00, 0x00, 0x01,
      127, 0, 0, 1,
      0, 0
    )
    assert(client:write(reply))
    -- remain silent: hold connection open for TLS / HTTP
  end)
end

--- SOCKS5 that accepts TCP but never sends method reply.
local function start_socks5_silent(port)
  return start_silent_server(port)
end

--- Run fn in a child coroutine; fail if it does not finish within deadline_ms.
local function run_with_deadline(deadline_ms, fn)
  local finished = false
  local hung = false
  local results = {}
  local call_err

  local wd = uv.timer.new()
  wd:start(function()
    if not finished then
      hung = true
    end
  end, deadline_ms)

  async.run(function()
    local ok, a, b = xpcall(function()
      return fn()
    end, debug.traceback)
    finished = true
    if ok then
      results[1], results[2] = a, b
    else
      call_err = a
    end
  end)

  while not finished and not hung do
    async.pause(20)
  end
  wd:stop()

  if hung then
    return nil, 'hung'
  end
  if call_err then
    return nil, call_err
  end
  return results[1], results[2]
end

TestHttpRequestTimeoutHang = {}

function TestHttpRequestTimeoutHang:tearDown()
  if self._server then
    self._server:stop()
    self._server = nil
  end
end

-- Baseline: plain HTTP to silent peer should respect timeout (TCP close wakes read).
function TestHttpRequestTimeoutHang:test_http_timeout_on_silent_server()
  local port = alloc_port()
  self._server = assert(start_silent_server(port))

  local resp, err = run_with_deadline(3000, function()
    local req = request.new{
      url = 'http://127.0.0.1:' .. port .. '/',
      timeout = 1,
    }
    return req:exec()
  end)

  lu.assertNotEquals(err, 'hung', 'HTTP request hung despite timeout')
  lu.assertNil(resp)
  lu.assertEquals(err, 'timeout')
end

-- Bug: HTTPS handshake waits on m_write_cont; ssl.close()/tcp.close() does not resume it.
-- With timeout=1, exec should return 'timeout' within ~2s; currently hangs forever.
function TestHttpRequestTimeoutHang:test_https_timeout_on_silent_server_hangs()
  local port = alloc_port()
  self._server = assert(start_silent_server(port))

  local resp, err = run_with_deadline(3000, function()
    local req = request.new{
      url = 'https://127.0.0.1:' .. port .. '/',
      timeout = 1,
    }
    return req:exec()
  end)

  -- Desired: timeout within deadline. Current bug: hung.
  lu.assertNotEquals(err, 'hung',
    'HTTPS request hung despite timeout (SSL handshake not cancelled by close)')
  lu.assertNil(resp)
  lu.assertEquals(err, 'timeout')
end

-- Silent SOCKS5 (no method reply): close should complete socks5 connect via on_closed.
function TestHttpRequestTimeoutHang:test_http_timeout_on_silent_socks5_proxy()
  local proxy_port = alloc_port()
  self._server = assert(start_socks5_silent(proxy_port))

  local resp, err = run_with_deadline(3000, function()
    local req = request.new{
      url = 'http://127.0.0.1:9/',
      proxy = 'socks5://127.0.0.1:' .. proxy_port,
      timeout = 1,
    }
    function req:resolve()
      self._ip_list = {{addr = '127.0.0.1', socktype = 'tcp'}}
      return true
    end
    return req:exec()
  end)

  lu.assertNotEquals(err, 'hung', 'HTTP via silent SOCKS5 hung despite timeout')
  lu.assertNil(resp)
  lu.assertEquals(err, 'timeout')
end

-- Bug repro with proxy: SOCKS5 CONNECT succeeds, then HTTPS handshake hangs;
-- request timeout closes sockets but SSL handshake coroutine is never resumed.
function TestHttpRequestTimeoutHang:test_https_timeout_via_socks5_after_connect_hangs()
  local proxy_port = alloc_port()
  self._server = assert(start_socks5_connect_ok_silent(proxy_port))

  local resp, err = run_with_deadline(3000, function()
    local req = request.new{
      url = 'https://127.0.0.1:443/',
      proxy = 'socks5://127.0.0.1:' .. proxy_port,
      timeout = 1,
    }
    function req:resolve()
      self._ip_list = {{addr = '127.0.0.1', socktype = 'tcp'}}
      return true
    end
    return req:exec()
  end)

  lu.assertNotEquals(err, 'hung',
    'HTTPS via SOCKS5 hung despite timeout (SSL handshake not cancelled)')
  lu.assertNil(resp)
  lu.assertEquals(err, 'timeout')
end


