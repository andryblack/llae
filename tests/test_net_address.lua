local lu = require 'luaunit'
local uv = require 'llae.uv'
local net = require 'net'

TestNetAddress = {}

function TestNetAddress:test_default_is_empty()
    local addr = net.address.new()
    lu.assertEquals(addr.type, net.address.ip_type.none)
    lu.assertEquals(addr.port, 0)
    lu.assertEquals(addr.scope_id, -1)
    lu.assertEquals(addr:get_ip_string(), '')
    lu.assertEquals(#addr:get_ip(), 0)
end

function TestNetAddress:test_ipv4_from_string()
    local addr = assert(net.address.from_string('127.0.0.1', 8080))
    lu.assertEquals(addr.type, net.address.ip_type.ipv4)
    lu.assertEquals(addr.port, 8080)
    lu.assertEquals(addr.scope_id, -1)
    lu.assertEquals(addr:get_ip_string(), '127.0.0.1')
    lu.assertEquals(addr:get_ip(), '\127\0\0\1')
end

function TestNetAddress:test_ipv6_from_string()
    local addr = assert(net.address.from_string('2001:db8:0:0::1', 443))
    lu.assertEquals(addr.type, net.address.ip_type.ipv6)
    lu.assertEquals(addr.port, 443)
    lu.assertEquals(addr.scope_id, -1)
    lu.assertEquals(addr:get_ip_string(), '2001:db8::1')
    lu.assertEquals(#addr:get_ip(), 16)
end

function TestNetAddress:test_ipv6_zone()
    local uname = assert(uv.os.uname())
    if uname.sysname:find('Windows') then
        lu.skip('numeric IPv6 scope ids are platform specific on Windows')
    end
    local ifname = uv.if_indextoname(1)
    if not ifname then
        lu.skip('no interface with index 1')
    end
    local addr = assert(net.address.from_string('fe80::1%' .. ifname, 53))
    lu.assertEquals(addr.type, net.address.ip_type.ipv6)
    lu.assertEquals(addr.scope_id, 1)
    lu.assertEquals(addr:get_ip_string(), 'fe80::1')
    lu.assertEquals(addr:get_ip_string_with_interface(), 'fe80::1%' .. ifname)
end

function TestNetAddress:test_from_string_invalid()
    local addr, err = net.address.from_string('not-an-ip', 80)
    lu.assertNil(addr)
    lu.assertStrContains(err, 'not-an-ip')
end

function TestNetAddress:test_from_string_unknown_zone()
    local addr, err = net.address.from_string('fe80::1%no-such-interface', 0)
    lu.assertNil(addr)
    lu.assertStrContains(err, 'zone')
end

function TestNetAddress:test_set_ip()
    local addr = net.address.new()
    assert(addr:set_ip(net.address.ip_type.ipv4, '\1\2\3\4'))
    lu.assertEquals(addr.type, net.address.ip_type.ipv4)
    lu.assertEquals(addr:get_ip_string(), '1.2.3.4')

    local ipv6 = string.rep('\0', 15) .. '\1'
    assert(addr:set_ip(net.address.ip_type.ipv6, ipv6))
    lu.assertEquals(addr.type, net.address.ip_type.ipv6)
    lu.assertEquals(addr:get_ip_string(), '::1')
end

function TestNetAddress:test_set_ip_rejects_wrong_size()
    local addr = net.address.new()
    local ok, err = addr:set_ip(net.address.ip_type.ipv4, 'xx')
    lu.assertNil(ok)
    lu.assertStrContains(err, 'IPv4')

    ok, err = addr:set_ip(net.address.ip_type.ipv6, '\1\2\3\4')
    lu.assertNil(ok)
    lu.assertStrContains(err, 'IPv6')
    lu.assertEquals(addr.type, net.address.ip_type.none)
end

function TestNetAddress:test_set_ipv4_clears_zone()
    local uname = assert(uv.os.uname())
    if uname.sysname:find('Windows') then
        lu.skip('numeric IPv6 scope ids are platform specific on Windows')
    end
    local ifname = uv.if_indextoname(1)
    if not ifname then
        lu.skip('no interface with index 1')
    end
    local addr = assert(net.address.from_string('fe80::1%' .. ifname, 1))
    lu.assertNotEquals(addr.scope_id, -1)
    assert(addr:set_ip(net.address.ip_type.ipv4, '\10\0\0\1'))
    lu.assertEquals(addr.scope_id, -1)
end

function TestNetAddress:test_port_is_mutable()
    local addr = assert(net.address.from_string('127.0.0.1', 1))
    addr.port = 65000
    lu.assertEquals(addr.port, 65000)
    lu.assertEquals(addr:get_ip_string(), '127.0.0.1')
end
