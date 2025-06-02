# HTTP Module

The `http` module provides functionality for creating HTTP servers and making HTTP requests. It supports both HTTP and HTTPS protocols, with automatic handling of SSL/TLS certificates.

## Basic Usage

### HTTP Server

```lua
local http = require 'net.http'

-- Create a simple HTTP server
local server = http.createServer(function(req, res)
    -- Handle request
    res:finish("Hello World!")
end)

-- Start listening
server:listen(8080, "127.0.0.1")
```

### HTTP Client

```lua
local http = require 'net.http'

-- Create and execute a request
local request = http.createRequest({
    url = "https://api.example.com/data",
    method = "GET"
})
local response = request:exec()
```

## Server API

### Creating a Server

```lua
local server = http.createServer(handler)
```

**Parameters:**
- `handler` (function): Callback function receiving (request, response) arguments

### Server Methods

#### `server:listen(port, address[, backlog])`
Starts the server listening for connections.

**Parameters:**
- `port` (number): TCP port to listen on
- `address` (string): IP address to bind to
- `backlog` (number, optional): Connection backlog size (default: 128)

**Returns:**
- `success` (boolean): true if successful
- `error` (string): Error message if failed

#### `server:stop()`
Stops the server and closes all connections.

### Request Object

The request object passed to handlers contains:

#### Properties
- `query` - Parsed query parameters
- `body` - Request body (if any)

#### Methods
- `request:get_header(name)` - Get header value
- `request:get_method()` - Get HTTP method
- `request:get_path()` - Get request path
- `request:get_protocol()` - Get protocol (HTTP/1.0 or HTTP/1.1)

### Response Object

The response object passed to handlers provides:

#### Methods

##### `response:set_header(name, value)`
Sets a response header.

```lua
response:set_header("Content-Type", "application/json")
```

##### `response:status(code[, message])`
Sets the response status code and optional message.

```lua
response:status(404, "Not Found")
```

##### `response:write(data)`
Writes data to the response body.

```lua
response:write("Hello ")
response:write("World!")
```

##### `response:finish([data])`
Ends the response, optionally writing final data.

```lua
response:finish("Done")
```

##### `response:send_file(path[, options])`
Sends a file with proper content type and optional compression.

```lua
response:send_file("public/index.html")
```

##### `response:keep_alive()`
Enables keep-alive connection.

## Client API

### Creating a Request

```lua
local request = http.createRequest({
    url = "https://api.example.com",
    method = "POST",
    headers = {
        ["Content-Type"] = "application/json"
    },
    body = '{"key": "value"}',
    timeout = 30 -- seconds
})
```

**Options:**
- `url` (string): Target URL
- `method` (string): HTTP method (default: "GET")
- `headers` (table): Request headers
- `body` (string): Request body
- `timeout` (number): Request timeout in seconds
- `version` (string): HTTP version (default: "1.1")

### Request Methods

#### `request:exec()`
Executes the request.

**Returns:**
- `response`: Response object if successful
- `error`: Error message if failed

```lua
local response, err = request:exec()
if err then
    print("Error:", err)
else
    print("Status:", response:get_code())
    print("Body:", response:read_body())
end
```

### Response Object

The response object returned from requests provides:

#### Methods
- `response:get_code()` - Get status code
- `response:get_header(name)` - Get response header
- `response:read_body()` - Read response body
- `response:close()` - Close the connection


## Examples

### Simple File Server

```lua
local http = require 'net.http'
local fs = require 'llae.fs'

local server = http.createServer(function(req, res)
    local path = "public" .. req:get_path()
    if fs.isfile(path) then
        return res:send_file(path)
    else
        return res:status(404,"Not Found")
    end
end)

server:listen(8080, "127.0.0.1")
```

### HTTP Client with JSON

```lua
local http = require 'net.http'
local json = require 'llae.json'

local request = http.createRequest({
    url = "https://api.example.com/data",
    method = "POST",
    headers = {
        ["Content-Type"] = "application/json"
    },
    body = json.encode({
        name = "test",
        value = 123
    })
})

local response = request:exec()
if response then
    local data = json.decode(response:read_body())
    print(data.result)
end
```

### Streaming Response

```lua
local http = require 'net.http'

local server = http.createServer(function(req, res)
    res:set_header("Content-Type", "text/plain")
    res:write("Part 1\n")
    res:write("Part 2\n")
    res:finish("Done")
end)

server:listen(8080, "127.0.0.1")
``` 