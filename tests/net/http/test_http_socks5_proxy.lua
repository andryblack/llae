local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'
local request = require 'net.http.request'

-- SOCKS5 proxy tests: with remote_dns the target host must go to the proxy as a
-- domain name (ATYP 0x03) so the proxy resolves it, otherwise as an IP address.

local next_port = 31100 + (os.time() % 1000)

local function alloc_port()
  next_port = next_port + 1
  return next_port
end

--- Buffered reader over an async connection: read(n) returns exactly n bytes.
---@param client uv.tcp_connection
---@return {read:fun(n:integer):string?,read_any:fun():any}
local function new_reader(client)
  local buf = ''
  local reader = {}
  function reader.read(n)
    while #buf < n do
      local chunk, err = client:read()
      if not chunk then
        return nil, err
      end
      buf = buf .. tostring(chunk)
    end
    local res = buf:sub(1, n)
    buf = buf:sub(n + 1)
    return res
  end
  function reader.read_any()
    if #buf > 0 then
      local res = buf
      buf = ''
      return res
    end
    return client:read()
  end
  return reader
end

--- Minimal SOCKS5 server: greets, answers CONNECT with success, records the
--- requested target and then replies with a tiny HTTP response.
---@param port integer
---@param auth {user:string,pass:string}? require username/password auth
---@return table?
---@return string?
local function start_socks5_http(port, auth)
  local server = uv.tcp_server.new()
  local ok, err = server:bind('127.0.0.1', port)
  if not ok then
    return nil, err
  end
  local state = {
    port = port,
    target = nil,
    auth = nil,
    _server = server,
    _clients = {},
    _stopped = false,
  }
  state._accept_thread = async.run(function()
    while not state._stopped do
      local res = server:listen(16)
      if not res then
        break
      end
      local client = uv.tcp_connection.new()
      local aok, aerr = server:accept(client)
      if aok then
        table.insert(state._clients, client)
        async.run(function()
          local reader = new_reader(client)
          -- method selection: VER NMETHODS METHODS
          local data = reader.read(2)
          if not data then
            client:close()
            return
          end
          local nmethods = data:byte(2)
          if not reader.read(nmethods) then
            client:close()
            return
          end

          if auth then
            client:write(string.char(0x05, 0x02))
            -- VER ULEN UNAME PLEN PASSWD
            data = reader.read(2)
            if not data then
              client:close()
              return
            end
            local ulen = data:byte(2)
            local rest = reader.read(ulen + 1)
            if not rest then
              client:close()
              return
            end
            local uname = rest:sub(1, ulen)
            local plen = rest:byte(ulen + 1)
            local pass = reader.read(plen)
            if not pass then
              client:close()
              return
            end
            state.auth = { user = uname, pass = pass }
            client:write(string.char(0x01, 0x00))
          else
            client:write(string.char(0x05, 0x00))
          end

          -- CONNECT request: VER CMD RSV ATYP DST.ADDR DST.PORT
          data = reader.read(5)
          if not data then
            client:close()
            return
          end
          local atyp = data:byte(4)
          local target = { atyp = atyp }
          if atyp == 0x03 then
            local len = data:byte(5)
            local rest = assert(reader.read(len + 2))
            target.host = rest:sub(1, len)
            target.port = rest:byte(len + 1) * 256 + rest:byte(len + 2)
          elseif atyp == 0x01 then
            local rest = assert(reader.read(5))
            target.host = string.format('%d.%d.%d.%d',
              data:byte(5), rest:byte(1), rest:byte(2), rest:byte(3))
            target.port = rest:byte(4) * 256 + rest:byte(5)
          else
            target.host = '?'
            target.port = 0
          end
          state.target = target

          -- VER REP RSV ATYP BND.ADDR BND.PORT
          client:write(string.char(0x05, 0x00, 0x00, 0x01, 127, 0, 0, 1, 0, 0))

          -- act as the target HTTP server
          local reqdata = reader.read_any()
          if reqdata then
            client:write(
              'HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok')
          end
          client:close()
        end)
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

--- Run fn in a child coroutine and wait for it, returning its results.
local function run_async(fn)
  local finished = false
  local results = {}
  async.run(function()
    local ok, a, b = xpcall(fn, debug.traceback)
    if ok then
      results[1], results[2] = a, b
    else
      results[1], results[2] = nil, a
    end
    finished = true
  end)
  while not finished do
    async.pause(10)
  end
  return results[1], results[2]
end

---@param port integer
---@param url string
---@return net.http.request_response?
---@return string?
local function exec_via_proxy(port, url)
  return run_async(function()
    local req = request.new{
      url = url,
      proxy = 'socks5://127.0.0.1:' .. port,
      remote_dns = true,
      timeout = 3,
    }
    return req:exec()
  end)
end

TestHttpSocks5Proxy = {}

function TestHttpSocks5Proxy:tearDown()
  if self._server then
    self._server:stop()
    self._server = nil
  end
end

function TestHttpSocks5Proxy:test_remote_dns_param_selects_resolve_side()
  local local_dns = request.new{
    url = 'http://example.com/',
    proxy = 'socks5://127.0.0.1:1080',
  }
  lu.assertFalse(local_dns._remote_dns)
  lu.assertFalse(local_dns:_resolve_on_proxy())

  -- the flag lives outside the proxy, on the request params
  local remote = request.new{
    url = 'http://example.com/',
    proxy = 'socks5://127.0.0.1:1080',
    remote_dns = true,
  }
  lu.assertTrue(remote._remote_dns)
  lu.assertTrue(remote:_resolve_on_proxy())

  -- remote mode must hand the host name to the proxy, not resolve it
  local res, err = remote:resolve()
  lu.assertTrue(res)
  lu.assertNil(err)
  lu.assertEquals(#remote._ip_list, 1)
  lu.assertEquals(remote._ip_list[1].addr, 'example.com')
  lu.assertEquals(remote._ip_list[1].socktype, 'tcp')

  -- without a proxy the flag has no effect
  local no_proxy = request.new{
    url = 'http://example.com/',
    remote_dns = true,
  }
  lu.assertFalse(no_proxy:_resolve_on_proxy())
end

function TestHttpSocks5Proxy:test_unknown_proxy_scheme_rejected()
  for _, proxy in ipairs{'http://127.0.0.1:1080','socks5h://127.0.0.1:1080'} do
    lu.assertErrorMsgContains('unsupported proxy', function()
      request.new{
        url = 'http://example.com/',
        proxy = proxy,
      }
    end)
  end
end

function TestHttpSocks5Proxy:test_remote_dns_sends_domain_to_proxy()
  local port = alloc_port()
  self._server = assert(start_socks5_http(port))

  -- .invalid never resolves locally, so a local lookup would fail the request
  local resp, err = exec_via_proxy(port, 'http://host.does.not.exist.invalid/')

  lu.assertNil(err,'request failed: ' .. tostring(err))
  lu.assertNotNil(resp)
  lu.assertEquals(resp:get_code(), 200)
  resp:close()

  lu.assertNotNil(self._server.target)
  lu.assertEquals(self._server.target.atyp, 0x03)
  lu.assertEquals(self._server.target.host, 'host.does.not.exist.invalid')
  lu.assertEquals(self._server.target.port, 80)
end

function TestHttpSocks5Proxy:test_socks5_sends_ip_to_proxy()
  local port = alloc_port()
  self._server = assert(start_socks5_http(port))

  local resp, err = run_async(function()
    local req = request.new{
      url = 'http://127.0.0.1/',
      proxy = 'socks5://127.0.0.1:' .. port,
      timeout = 3,
    }
    return req:exec()
  end)

  lu.assertNil(err,'request failed: ' .. tostring(err))
  lu.assertNotNil(resp)
  lu.assertEquals(resp:get_code(), 200)
  resp:close()

  lu.assertNotNil(self._server.target)
  lu.assertEquals(self._server.target.atyp, 0x01)
  lu.assertEquals(self._server.target.host, '127.0.0.1')
end

function TestHttpSocks5Proxy:test_remote_dns_with_auth_sends_domain_to_proxy()
  local port = alloc_port()
  self._server = assert(start_socks5_http(port,{ user = 'user', pass = 'secret' }))

  local resp, err = run_async(function()
    local req = request.new{
      url = 'http://auth.host.invalid/',
      proxy = 'socks5://user:secret@127.0.0.1:' .. port,
      remote_dns = true,
      timeout = 3,
    }
    return req:exec()
  end)

  lu.assertNil(err,'request failed: ' .. tostring(err))
  lu.assertNotNil(resp)
  lu.assertEquals(resp:get_code(), 200)
  resp:close()

  lu.assertNotNil(self._server.auth)
  lu.assertEquals(self._server.auth.user, 'user')
  lu.assertEquals(self._server.auth.pass, 'secret')
  lu.assertNotNil(self._server.target)
  lu.assertEquals(self._server.target.atyp, 0x03)
  lu.assertEquals(self._server.target.host, 'auth.host.invalid')
end
