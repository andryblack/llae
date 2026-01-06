local lu = require 'luaunit'
local uv = require 'llae.uv'

TestUV = {}

function TestUV:_check_ip4(addr_str)
    local addr = assert(uv.ip4_addr(addr_str))
    local ip = assert(uv.ip4_name(addr))
    lu.assertEquals(ip, addr_str)
end

function TestUV:test_ip4_addr()
    self:_check_ip4('127.0.0.1')
    self:_check_ip4('192.168.1.1')
    self:_check_ip4('0.0.0.0')
    self:_check_ip4('255.255.255.255')
end

function TestUV:test_ip4_addr_binary()
    local ip = assert(uv.ip4_addr('1.2.3.4'))
    lu.assertEquals(ip, '\x01\x02\x03\x04')
end

function TestUV:_check_ip6(addr_str,check_str)
    local addr = assert(uv.ip6_addr(addr_str))
    local ip = assert(uv.ip6_name(addr))
    lu.assertEquals(ip, check_str or addr_str)
end

function TestUV:test_ip6_addr()
    self:_check_ip6('::1')
    self:_check_ip6('2001:db8:85a3::8a2e:370:7334')
    self:_check_ip6('2001:db8:85a3:0:0:8a2e:370:7334','2001:db8:85a3::8a2e:370:7334')
end

function TestUV:test_ip6_addr_binary()
    local ip = assert(uv.ip6_addr('0102:0304:0506:0708:090a:0b0c:0d0e:0f10'))
    lu.assertEquals(ip, '\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10')
end

