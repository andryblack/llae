local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'

TestAsync = {}

function TestAsync:test_async_wait()

    local wait = uv.async.new()

    local emmit_called = false

    async.run(function()
        async.pause(100)
        emmit_called = true
        wait:emmit()
    end)

    lu.assertFalse(emmit_called)
    wait:wait()
    lu.assertTrue(emmit_called)
    wait:close()
end
