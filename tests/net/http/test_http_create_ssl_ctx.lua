local lu = require 'luaunit'
local http = require 'net.http'

TestHttpCreateSslCtx = {}

function TestHttpCreateSslCtx:test_creates_new_context()
    local a = http.create_ssl_ctx(false)
    local b = http.create_ssl_ctx(false)

    lu.assertNotNil(a)
    lu.assertEquals(type(a.load_cert),'function')
    lu.assertNotEquals(a,b)
end

function TestHttpCreateSslCtx:test_loads_default_certs()
    lu.assertNotNil(http.create_ssl_ctx(true))
end

function TestHttpCreateSslCtx:test_can_skip_default_certs()
    lu.assertNotNil(http.create_ssl_ctx(false))
end

function TestHttpCreateSslCtx:test_shared_context_is_cached()
    lu.assertEquals(http.get_ssl_ctx(),http.get_ssl_ctx())
end
