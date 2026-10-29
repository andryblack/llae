local lu = require 'luaunit'
local ssl = require 'ssl'
local http = require 'net.http'
local request = require 'net.http.request'

TestHttpRequestSslCtx = {}

function TestHttpRequestSslCtx:test_custom_ssl_ctx()
    local ctx = ssl.ctx.new()
    assert(ctx:init('request-ssl-ctx-test'))

    local req = request.new{
        url = 'https://example.com/',
        ssl_ctx = ctx,
    }

    lu.assertEquals(req._ssl_ctx, ctx)
    lu.assertEquals(req:get_ssl_ctx(), ctx)
end

function TestHttpRequestSslCtx:test_ssl_ctx_is_optional()
    local req = request.new{
        url = 'https://example.com/',
    }

    lu.assertNil(req._ssl_ctx)
    lu.assertEquals(req:get_ssl_ctx(), http.get_ssl_ctx())
end

function TestHttpRequestSslCtx:test_class_get_ssl_ctx_keeps_working()
    lu.assertEquals(request.get_ssl_ctx(), http.get_ssl_ctx())
end
