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

-- Every awaited async call must release its promise: the C frame that starts
-- the await is discarded by lua_yieldk, so nothing there may own the promise.
function TestAsync:test_await_does_not_leak_objects()
    local llae = require 'llae'
    -- warm up lazily allocated objects of the pause machinery
    uv.pause(0)
    collectgarbage()
    collectgarbage()
    local before = llae.debug_object_count()
    for _ = 1, 300 do
        uv.pause(0)
    end
    collectgarbage()
    collectgarbage()
    local after = llae.debug_object_count()
    lu.assertTrue(after <= before,
        ('await leaks meta objects: before=%d after=%d'):format(before, after))
end
