local lu = require 'luaunit'
local async = require 'llae.async'
local uv = require 'llae.uv'

TestAsync = {}

function TestAsync:test_async_wait()

    local wait = uv.async.new()

    async.run(function()
        async.pause(100)
        wait:emmit()
    end)

    wait:wait()
end
