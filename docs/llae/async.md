# Async Module

The `async` module provides functionality for asynchronous programming using coroutines. It includes utilities for coroutine management, locks for synchronization, and event handling.

## Core Functions

### `async.pause`
Pauses the event loop. This is a direct binding to `uv.pause` from the libuv library.

### `async.resume(thread)`
Resumes a coroutine and handles any errors that occur during execution.

**Parameters:**
- `thread` (thread): The coroutine to resume

**Throws:**
- Error if the coroutine fails to resume, with detailed stack trace

**Example:**
```lua
local async = require 'llae.async'
local thread = coroutine.create(function()
    -- Some async work
end)
async.resume(thread)
```

### `async.run(fn, handle_error)`
Creates and starts a new coroutine to run the given function.

**Parameters:**
- `fn` (function): The function to run in a new coroutine
- `handle_error` (boolean, optional): If true, wraps the function in error handling code

**Example:**
```lua
local async = require 'llae.async'

-- Simple execution
async.run(function()
    -- Async work here
end)

-- With error handling
async.run(function()
    -- Async work that might throw
end, true)
```

## Lock Class

The `Lock` class provides mutual exclusion functionality for coroutines.

### Creating a Lock
```lua
local async = require 'llae.async'
local lock = async.lock.new()
```

### Methods

#### `lock:lock()`
Acquires the lock. If the lock is already held, waits until it's released.

**Example:**
```lua
async.run(function()
    lock:lock()
    -- Critical section
    lock:unlock()
end)
```

#### `lock:unlock()`
Releases the lock and wakes up one waiting coroutine if any.

#### `lock:wait_unlock()`
Waits for the lock to be released. This is used internally by the `lock` method.

#### `lock:report_unlock()`
Internal method that resumes the next waiting coroutine.

**Example of Lock Usage:**
```lua
local async = require 'llae.async'
local lock = async.lock.new()

-- Coroutine 1
async.run(function()
    lock:lock()
    -- Critical section
    lock:unlock()
end)

-- Coroutine 2
async.run(function()
    lock:lock()
    -- Will wait until Coroutine 1 unlocks
    -- Critical section
    lock:unlock()
end)
```

## Event Class

The `Event` class provides a way to synchronize coroutines using events.

### Creating an Event
```lua
local async = require 'llae.async'
local event = async.event.new()
```

### Methods

#### `event:set()`
Sets the event and wakes up all waiting coroutines.

#### `event:clear()`
Clears the event state.

#### `event:wait()`
Waits for the event to be set. If the event is already set, returns immediately.

**Example of Event Usage:**
```lua
local async = require 'llae.async'
local event = async.event.new()

-- Waiting coroutine
async.run(function()
    print("Waiting for event...")
    event:wait()
    print("Event received!")
end)

-- Signaling coroutine
async.run(function()
    -- Do some work
    print("Setting event...")
    event:set()
end)
```

## Complete Example

Here's a complete example showing how to use locks and events together:

```lua
local async = require 'llae.async'
local log = require 'llae.log'

-- Create synchronization primitives
local lock = async.lock.new()
local ready_event = async.event.new()

-- Worker coroutine
async.run(function()
    lock:lock()
    log.info("Worker: Processing...")
    -- Simulate work
    lock:unlock()
    ready_event:set()
end)

-- Monitor coroutine
async.run(function()
    log.info("Monitor: Waiting for worker...")
    ready_event:wait()
    log.info("Monitor: Worker completed")
end)
```

## Best Practices

1. Always pair `lock:lock()` with `lock:unlock()` in the same coroutine
2. Use `async.run` with `handle_error = true` for better error handling in production code
3. Clear events when they need to be reused
4. Be careful with shared resources in concurrent coroutines
5. Use locks for mutual exclusion and events for signaling between coroutines 