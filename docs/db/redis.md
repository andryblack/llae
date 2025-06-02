# Redis Module

The `redis` module provides an asynchronous Redis client implementation using coroutines. It supports all standard Redis commands, pipelining, and pub/sub functionality.

## Basic Usage

### Creating a Connection

```lua
local redis = require 'db.redis'
local client = redis.new()

-- Connect using TCP
local ok, err = client:connect("127.0.0.1", 6379)

-- Or connect using Unix socket
local ok, err = client:connect("/path/to/redis.sock")
```

### Basic Commands

The module supports all standard Redis commands in their lowercase form:

```lua
-- String operations
local value = client:get("key")
client:set("key", "value")

-- Hash operations
client:hset("hash", "field", "value")
local value = client:hget("hash", "field")

-- List operations
client:lpush("list", "value")
local value = client:lpop("list")

-- Set operations
client:sadd("set", "member")
local exists = client:sismember("set", "member")

-- Sorted set operations
client:zadd("sorted", 1, "member")
local members = client:zrange("sorted", 0, -1)
```

## Advanced Features

### Command Execution

#### `redis:cmd(command, ...)`
Executes a Redis command and throws an error if it fails.

**Parameters:**
- `command` (string): Redis command name
- `...`: Command arguments

**Returns:**
- Command result
- Throws error on failure

**Example:**
```lua
local result = client:cmd("SET", "key", "value")
```

#### `redis:try_cmd(command, ...)`
Executes a Redis command and returns error instead of throwing.

**Parameters:**
- `command` (string): Redis command name
- `...`: Command arguments

**Returns:**
- `result`: Command result or nil on error
- `error`: Error message if command failed

**Example:**
```lua
local result, err = client:try_cmd("GET", "key")
if err then
    print("Error:", err)
end
```

### Pipelining

#### `redis:pipelining(encoded_commands)`
Executes multiple commands in a pipeline.

**Parameters:**
- `encoded_commands` (table): Array of encoded Redis commands

**Returns:**
- `results` (table): Array of command results
- `error`: Error message if pipeline failed

**Example:**
```lua
local commands = {
    redis.encode("SET", "key1", "value1"),
    redis.encode("SET", "key2", "value2"),
    redis.encode("MGET", "key1", "key2")
}
local results = client:pipelining(commands)
```

### Pub/Sub

#### `redis:subscribe(channel, handler)`
Subscribes to a Redis channel.

**Parameters:**
- `channel` (string): Channel name
- `handler` (function): Callback function receiving (channel, message)

**Example:**
```lua
client:subscribe("news", function(channel, message)
    print("Received from " .. channel .. ": " .. message)
end)
```

#### `redis:unsubscribe(...)`
Unsubscribes from one or more channels.

**Parameters:**
- `...` (strings): Channel names. If none provided, unsubscribes from all channels.

**Example:**
```lua
-- Unsubscribe from specific channel
client:unsubscribe("news")

-- Unsubscribe from all channels
client:unsubscribe()
```

### Connection Management

#### `redis:close()`
Closes the Redis connection.

**Example:**
```lua
client:close()
```

## Supported Commands

The module supports the following Redis commands (called as methods in lowercase):

### Strings
- `get`, `set`, `del`
- `incr`, `decr`
- `mget`, `mset`

### Lists
- `llen`, `lindex`, `lpop`, `rpop`
- `lpush`, `rpush`
- `lrange`, `linsert`
- `ltrim`, `lrem`, `lset`

### Hashes
- `hexists`, `hget`, `hgetall`
- `hset`, `hsetnx`
- `hmget`, `hmset`
- `hdel`, `hincrby`
- `hkeys`, `hlen`
- `hstrlen`, `hvals`

### Sets
- `smembers`, `sismember`
- `sadd`, `srem`
- `sdiff`, `sinter`, `sunion`
- `srandmember`

### Sorted Sets
- `zrange`, `zrangebyscore`
- `zrank`
- `zadd`, `zrem`
- `zincrby`

### Bit Operations
- `bitfield`, `bitcount`
- `bitop`, `bitpos`
- `getbit`, `setbit`

### Other
- `auth`, `eval`, `script`
- `sort`, `scan`
- `expire`, `persist`
- `publish`
- `rename`

## Best Practices

1. Always handle connection errors:
```lua
local ok, err = client:connect("127.0.0.1", 6379)
if not ok then
    error("Failed to connect: " .. err)
end
```

2. Use `try_cmd` when you want to handle errors gracefully:
```lua
local value, err = client:try_cmd("GET", "key")
if err then
    -- Handle error
else
    -- Use value
end
```

3. Use pipelining for multiple commands to improve performance:
```lua
local results = client:pipelining({
    redis.encode("SET", "key1", "value1"),
    redis.encode("SET", "key2", "value2")
})
```

4. Always close the connection when done:
```lua
local function cleanup()
    client:close()
end
```

5. Handle pub/sub disconnections:
```lua
client:subscribe("channel", function(channel, message)
    if not message then
        -- Handle disconnection
        return
    end
    -- Process message
end)
``` 