local lu = require 'luaunit'
local async = require 'llae.async'
local log = require 'llae.log'

TestAsyncLock = {}


function TestAsyncLock:test_lock_blocks_second_coroutine()
  local lock = async.lock.new()
  local order = {}
  
  local function checkpoint(name)
    table.insert(order, name)
    log.info(name)
  end
  local t
  do
    lock:lock()
    checkpoint('first_acquired')
    -- Hold lock for a bit
    t = async.run(function()
      checkpoint('second_waiting')
      lock:lock()
      checkpoint('second_acquired')
      lock:unlock()
    end)
    -- Release lock after second coroutine starts waiting
    checkpoint('first_releasing')
    lock:unlock()
  end

  t:join()
  
  lu.assertEquals(order[1], 'first_acquired')
  lu.assertEquals(order[2], 'second_waiting')
  lu.assertEquals(order[3], 'first_releasing')
  lu.assertEquals(order[4], 'second_acquired')
end

function TestAsyncLock:test_multiple_waiters_fifo_order()
  local lock = async.lock.new()
  local order = {}
  ---@type llae.async.thread[]
  local threads = {}
  
  local main_thread = async.run(function()
    lock:lock()
    table.insert(order, 'holder_acquired')
    
    -- Start three waiting coroutines
    threads[1] = async.run(function()
      table.insert(order, 'waiter1_start')
      lock:lock()
      table.insert(order, 'waiter1_acquired')
      lock:unlock()
    end)
    
    threads[2] = async.run(function()
      table.insert(order, 'waiter2_start')
      lock:lock()
      table.insert(order, 'waiter2_acquired')
      lock:unlock()
    end)
    
    threads[3] = async.run(function()
      table.insert(order, 'waiter3_start')
      lock:lock()
      table.insert(order, 'waiter3_acquired')
      lock:unlock()
    end)
    
    -- Release lock
    table.insert(order, 'holder_releasing')
    lock:unlock()
  end)
  
  main_thread:join()
  for _, t in ipairs(threads) do
    t:join()
  end
  
  -- Verify FIFO order
  lu.assertEquals(order[1], 'holder_acquired')
  lu.assertEquals(order[2], 'waiter1_start')
  lu.assertEquals(order[3], 'waiter2_start')
  lu.assertEquals(order[4], 'waiter3_start')
  lu.assertEquals(order[5], 'holder_releasing')
  lu.assertEquals(order[6], 'waiter1_acquired')
  lu.assertEquals(order[7], 'waiter2_acquired')
  lu.assertEquals(order[8], 'waiter3_acquired')
end

function TestAsyncLock:test_nested_lock_same_coroutine()
  local lock = async.lock.new()
  local error_occurred = false
  local error_message = nil
  
  local main_thread = async.run(function()
    lock:lock()
    
    -- Try to lock again in same coroutine - this should throw an error
    local ok, err = pcall(function()
      lock:lock()
    end)
    if not ok then
      error_occurred = true
      error_message = tostring(err)
    end
    
    lock:unlock()
  end)
  
  main_thread:join()
  lu.assertTrue(error_occurred, "Expected error when locking same lock twice in same coroutine")
  lu.assertNotNil(error_message)
  lu.assertStrContains(error_message, "lock called by a coroutine that already holds the lock")
end

function TestAsyncLock:test_multiple_independent_locks()
  local lock1 = async.lock.new()
  local lock2 = async.lock.new()
  local order = {}
  ---@type llae.async.thread
  local second_thread
  
  local main_thread = async.run(function()
    lock1:lock()
    table.insert(order, 'lock1_acquired')
    
    second_thread = async.run(function()
      lock2:lock()
      table.insert(order, 'lock2_acquired')
      lock2:unlock()
    end)
    
    lock1:unlock()
    table.insert(order, 'lock1_released')
  end)
  
  main_thread:join()
  second_thread:join()
  
  -- lock2 should be acquired even though lock1 is held
  lu.assertEquals(order[1], 'lock1_acquired')
  lu.assertEquals(order[2], 'lock2_acquired')
  lu.assertEquals(order[3], 'lock1_released')
end

function TestAsyncLock:test_lock_protects_shared_resource()
  local lock = async.lock.new()
  local event = async.event.new()
  local counter = 0
  local completed = 0
  local num_coroutines = 5
  local iterations = 100
  ---@type llae.async.thread[]
  local threads = {}
  
  local main_thread = async.run(function()
    -- Start multiple coroutines that increment counter
    for i = 1, num_coroutines do
      threads[i] = async.run(function()
        for j = 1, iterations do
          lock:lock()
          local old = counter
          -- Simulate some work
          counter = old + 1
          lock:unlock()
        end
        completed = completed + 1
        if completed == num_coroutines then
          event:set()
        end
      end)
    end
    -- Wait for all coroutines to complete
    event:wait()
  end)
  
  main_thread:join()
  for _, t in ipairs(threads) do
    t:join()
  end
  
  lu.assertEquals(counter, num_coroutines * iterations)
  lu.assertEquals(completed, num_coroutines)
end

function TestAsyncLock:test_unlock_without_lock()
  local lock = async.lock.new()
  local error_occurred = false
  local error_message = nil
  
  local t = async.run(function()
    -- Try to unlock without locking first - this should throw an error
    local ok, err = pcall(function()
      lock:unlock()
    end)
    if not ok then
      error_occurred = true
      error_message = tostring(err)
    end
  end)
  
  t:join()
  lu.assertTrue(error_occurred, "Expected error when unlocking without holding the lock")
  lu.assertNotNil(error_message)
  lu.assertStrContains(error_message, "unlock called by a coroutine that does not hold the lock")
end

function TestAsyncLock:test_unlock_from_different_coroutine()
  local lock = async.lock.new()
  local event = async.event.new()
  local error_occurred = false
  local error_message = nil
  
  local main_thread = async.run(function()
    lock:lock()
    event:set()  -- Signal that lock is acquired
    async.pause(100)  -- Hold lock for a bit
    lock:unlock()
  end)
  
  local unlock_thread = async.run(function()
    event:wait()  -- Wait for main thread to acquire lock
    -- Try to unlock from different coroutine - this should throw an error
    local ok, err = pcall(function()
      lock:unlock()
    end)
    if not ok then
      error_occurred = true
      error_message = tostring(err)
    end
  end)
  
  unlock_thread:join()
  main_thread:join()
  lu.assertTrue(error_occurred, "Expected error when unlocking from different coroutine")
  lu.assertNotNil(error_message)
  lu.assertStrContains(error_message, "unlock called by a coroutine that does not hold the lock")
end

function TestAsyncLock:test_sequential_lock_unlock()
  local lock = async.lock.new()
  local order = {}
  
  local t = async.run(function()
    for i = 1, 3 do
      lock:lock()
      table.insert(order, 'lock_' .. i)
      lock:unlock()
      table.insert(order, 'unlock_' .. i)
    end
  end)
  
  t:join()
  lu.assertEquals(order, {
    'lock_1', 'unlock_1',
    'lock_2', 'unlock_2',
    'lock_3', 'unlock_3'
  })
end

function TestAsyncLock:test_lock_initialization()
  local lock = async.lock.new()
  
  -- Lock should be unlocked initially
  local acquired = false
  local t = async.run(function()
    lock:lock()
    acquired = true
    lock:unlock()
  end)
  
  t:join()
  lu.assertTrue(acquired)
end

