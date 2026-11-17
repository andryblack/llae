local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'
local net = require 'net'

TestUdpAddress = {}

function TestUdpAddress:test_bind_and_getsockname()
    local udp = uv.udp.new()
    assert(udp:bind('::', 0))
    local bound = assert(udp:getsockname())
    lu.assertEquals(bound:get_ip_string(), '::')
    lu.assertIsNumber(bound.port)
    udp:close()
end

-- A dual-stack UDP socket receives IPv4 datagrams as `::ffff:a.b.c.d`.
-- recv()/getpeername() must report the real address, not 0.0.0.0.
function TestUdpAddress:test_recv_reports_real_ipv4_peer()
    local server = uv.udp.new()
    assert(server:bind(net.address.from_string('::', 0)))
    local port = assert(server:getsockname()).port

    local client = uv.udp.new()
    async.run(function()
        assert(client:send('ping', net.address.from_string('127.0.0.1', port)))
    end)

    local data, from = server:recv()
    lu.assertNotNil(data)
    lu.assertNotNil(from)
    lu.assertEquals(tostring(data), 'ping')
    lu.assertEquals(from:get_ip_string(), '::ffff:127.0.0.1')
    lu.assertIsNumber(from.port)

    server:close()
    client:close()
end

function TestUdpAddress:test_send_with_address_object()
    local server = uv.udp.new()
    assert(server:bind('127.0.0.1', 0))
    local port = assert(server:getsockname()).port

    local client = uv.udp.new()
    async.run(function()
        assert(client:send('ping', net.address.from_string('127.0.0.1', port)))
    end)

    local data, from = server:recv()
    lu.assertNotNil(data)
    lu.assertNotNil(from)
    lu.assertEquals(tostring(data), 'ping')
    lu.assertEquals(from:get_ip_string(), '127.0.0.1')
    lu.assertIsNumber(from.port)

    -- echo back using the received address
    assert(server:send('pong', from))

    server:close()
    client:close()
end

function TestUdpAddress:test_send_without_port_fails()
    local udp = uv.udp.new()
    assert(udp:bind('127.0.0.1', 0))
    local ok, err = udp:send('x', '127.0.0.1')
    lu.assertNil(ok)
    lu.assertStrContains(err, 'port')
    udp:close()
end

function TestUdpAddress:test_getpeername_reports_error()
    local udp = uv.udp.new()
    local addr, err = udp:getpeername()
    lu.assertNil(addr)
    lu.assertIsString(err)
    udp:close()
end

function TestUdpAddress:test_recv_stopped_returns_error()
    local udp = uv.udp.new()
    assert(udp:bind('127.0.0.1', 0))
    local result = {}
    async.run(function()
        local data, err = udp:recv()
        result.data = data
        result.err = err
    end)
    udp:stop_recv()
    uv.pause(0)
    lu.assertNil(result.data)
    lu.assertIsString(result.err)
    udp:close()
end

function TestUdpAddress:test_recv_conflict_reports_error()
    local udp = uv.udp.new()
    assert(udp:bind('127.0.0.1', 0))
    async.run(function()
        udp:recv()
    end)
    local data, err = udp:recv()
    lu.assertNil(data)
    lu.assertStrContains(err, 'already recv')
    udp:stop_recv()
    udp:close()
end
