# PostgreSQL Module

The `pgsql` module provides an asynchronous PostgreSQL client implementation using coroutines. It supports all standard PostgreSQL operations, including authentication methods, queries, and data type conversions.

## Basic Usage

### Configuration and Connection

```lua
local pgsql = require 'db.pgsql'

-- Create client with default configuration
local client = pgsql.new()

-- Or with custom configuration
local client = pgsql.new({
    host = '127.0.0.1',
    port = 5432,
    database = 'mydb',
    user = 'myuser',
    password = 'mypassword',
    application_name = 'my-app' -- optional
})

-- Connect to database
local ok, err = client:connect()
if not ok then
    error("Failed to connect: " .. err)
end
```

### Default Configuration

```lua
pgsql.default_config = {
    database = 'postgres',
    user = 'postgres',
    host = '127.0.0.1',
    port = 5432,
    ssl = false
}
```

## Query Execution

### Simple Queries

```lua
-- Execute a simple query
local result, err = client:query("SELECT * FROM users WHERE id = 1")
if err then
    error("Query failed: " .. err)
end

-- Insert data
local result, err = client:query([[
    INSERT INTO users (name, email) 
    VALUES ('John', 'john@example.com')
    RETURNING id
]])

-- Update data
local result, err = client:query([[
    UPDATE users 
    SET name = 'Jane' 
    WHERE id = 1
]])

-- Delete data
local result, err = client:query("DELETE FROM users WHERE id = 1")
```

### Query Results

Query results are returned in different formats depending on the query type:

1. SELECT queries return an array of rows:
```lua
local users = client:query("SELECT * FROM users")
-- Result: 
-- {
--   { id = 1, name = "John", email = "john@example.com" },
--   { id = 2, name = "Jane", email = "jane@example.com" }
-- }
```

2. INSERT/UPDATE/DELETE queries return affected rows count:
```lua
local result = client:query("DELETE FROM users WHERE active = false")
-- Result: 
-- { affected_rows = 5 }
```

## Data Types

The module automatically converts PostgreSQL data types to Lua types:

### Basic Types
- `integer`, `bigint` → `number`
- `text`, `varchar`, `char` → `string`
- `boolean` → `boolean`
- `json`, `jsonb` → Lua table (automatically decoded)
- `bytea` → Lua string (automatically decoded)

### Array Types
- `boolean[]` → array of booleans
- `integer[]`, `bigint[]` → array of numbers
- `text[]`, `varchar[]` → array of strings

## Error Handling

Errors are returned with detailed information:

```lua
local result, err = client:query("SELECT * FROM non_existent_table")
if err then
    -- err contains:
    -- - Severity
    -- - Message
    -- - Position (if applicable)
    -- - Detail (if available)
    print(err)
end
```

## Connection Management

### Closing Connection

Always close the connection when done:

```lua
client:disconnect()
```

## Binary Data

The module provides utilities for handling binary data:

```lua
-- Encode binary data for queries
local encoded = pgsql.encode_bytea(binary_data)
client:query("INSERT INTO files (data) VALUES (" .. encoded .. ")")

-- Binary data is automatically decoded in query results
local result = client:query("SELECT data FROM files")
local binary_data = result[1].data
```

## Best Practices

1. Always handle connection errors:
```lua
local ok, err = client:connect()
if not ok then
    error("Failed to connect: " .. err)
end
```

2. Use proper error handling for queries:
```lua
local result, err = client:query("SELECT * FROM users")
if not result then
    -- Handle error
else
    -- Process result
end
```

3. Close connections when done:
```lua
local function cleanup()
    client:disconnect()
end
```

4. Use proper parameter escaping:
```lua
-- DON'T: Vulnerable to SQL injection
client:query("SELECT * FROM users WHERE name = '" .. user_input .. "'")

-- DO: Use proper parameter quoting or prepared statements
local name = "'" .. user_input:gsub("'", "''") .. "'"
client:query("SELECT * FROM users WHERE name = " .. name)
```

5. Handle binary data properly:
```lua
-- Use bytea encoding for binary data
local encoded = pgsql.encode_bytea(binary_data)
client:query("INSERT INTO files (data) VALUES (" .. encoded .. ")")
``` 