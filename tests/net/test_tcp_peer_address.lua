local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'
local net = require 'net'

TestTcpPeerAddress = {}

-- A dual-stack listener (`::`) accepts IPv4 clients as `::ffff:a.b.c.d`.
-- getpeername()/getsockname() must report the real address, not 0.0.0.0.
function TestTcpPeerAddress:test_ipv4_peer_on_dual_stack_listener()
    local srv = uv.tcp_server.new()
    assert(srv:bind(net.address.from_string('::', 0)))
    local bound = assert(srv:getsockname())
    lu.assertEquals(bound:get_ip_string(), '::')
    local port = bound.port
    lu.assertIsNumber(port)

    local client
    async.run(function()
        client = uv.tcp_connection.new()
        assert(client:connect('127.0.0.1', port))
    end)

    assert(srv:listen(8))
    local conn = uv.tcp_connection.new()
    assert(srv:accept(conn))

    local peer = assert(conn:getpeername())
    lu.assertEquals(peer.type, net.address.ip_type.ipv6)
    lu.assertEquals(peer:get_ip_string(), '::ffff:127.0.0.1')
    lu.assertIsNumber(peer.port)

    local conn_local = assert(conn:getsockname())
    lu.assertEquals(conn_local.port, port)

    local client_local = assert(client:getsockname())
    lu.assertEquals(client_local:get_ip_string(), '127.0.0.1')
    lu.assertEquals(client_local.port, peer.port)

    local client_peer = assert(client:getpeername())
    lu.assertEquals(client_peer:get_ip_string(), '127.0.0.1')
    lu.assertEquals(client_peer.port, port)

    conn:close()
    client:close()
    srv:stop()
end

function TestTcpPeerAddress:test_connect_with_address_object()
    local srv = uv.tcp_server.new()
    assert(srv:bind('127.0.0.1', 0))
    local port = assert(srv:getsockname()).port

    local client
    async.run(function()
        client = uv.tcp_connection.new()
        -- explicit port overrides the empty port stored in the address
        assert(client:connect(net.address.from_string('127.0.0.1', 0), port))
    end)

    assert(srv:listen(8))
    local conn = uv.tcp_connection.new()
    assert(srv:accept(conn))

    local peer = assert(conn:getpeername())
    lu.assertEquals(peer:get_ip_string(), '127.0.0.1')

    conn:close()
    client:close()
    srv:stop()
end
