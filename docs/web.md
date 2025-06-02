# Web Module

The `web` module provides a flexible and powerful web application framework for LLAE. It is inspired by Express.js and provides similar middleware-based architecture.

## Basic Usage

```lua
local web = require 'web.application'

-- Create application
local app = web.new()

-- Add middleware
app:use(web.static('./public'))
app:use(web.views('./views', {
    env = { title = 'My App' }
}))

-- Define routes
app:get('/', function(req, res)
    return res:render('index', { message = 'Hello World!' })
end)

-- Start server
app:listen({ port = 8080 })
```

## Application API

### Creating an Application

```lua
local app = web.new()
```

### Methods

#### `app:use(middleware)`
Adds middleware to the application.

```lua
app:use(web.static('./public'))
```

#### `app:listen(options)`
Starts the HTTP server.

**Options:**
- `port` (number): Port to listen on (default: 8080)
- `host` (string): Host to bind to (default: "127.0.0.1")
- `backlog` (number): Connection backlog size

#### `app:stop()`
Stops the HTTP server.

### Routing

The application supports Express-style routing with path parameters:

```lua
-- Basic route
app:get('/hello', function(req, res)
    return res:finish('Hello World!')
end)

-- Route with parameters
app:get('/users/:id', function(req, res)
    local id = req.params.id
    return res:finish('User ' .. id)
end)

-- Multiple HTTP methods
app:post('/api/data', handler)
app:put('/api/data/:id', handler)
app:delete('/api/data/:id', handler)
```

## Built-in Middleware

### Static Files

Serves static files from a directory:

```lua
app:use(web.static('./public', {
    path = '/static',  -- URL prefix
    extensions = {'html', 'css', 'js'}  -- Allowed extensions
}))
```

### Views

Template rendering with support for layouts and partials:

```lua
app:use(web.views('./views', {
    ext = 'thtml',  -- Template file extension
    env = {  -- Global template variables
        title = 'My App',
        version = '1.0'
    }
}))

-- In route handler:
app:get('/', function(req, res)
    return res:render('index', {
        message = 'Hello World!'
    })
end)
```

### JSON

Adds JSON request/response handling:

```lua
app:use(web.json())

app:post('/api/data', function(req, res)
    -- req.json is automatically parsed JSON
    return res:json({
        status = 'ok',
        data = req.json
    })
end)
```

### Cookie Parser

Adds cookie support:

```lua
app:use(web.cookie())

app:get('/', function(req, res)
    -- Read cookies
    local user = req.cookies.user
    
    -- Set cookie
    res:cookie('session', 'abc123', {
        maxAge = 3600,
        httpOnly = true
    })
end)
```

### Form Parser

Handles URL-encoded form data:

```lua
app:use(web.formparser())

app:post('/login', function(req, res)
    -- req.form contains form fields
    local username = req.form.username
    local password = req.form.password
end)
```

### Multipart

Handles multipart/form-data for file uploads:

```lua
app:use(web.multipart())

app:post('/upload', function(req, res)
    for _, file in ipairs(req.multipart) do
        print(file.name, file.filename, #file.data)
    end
end)
```

## Request Object

The request object provides:

### Properties
- `query` - Parsed query parameters
- `params` - Route parameters

### Methods
- `req:get_header(name)` - Get request header
- `req:get_method()` - Get HTTP method
- `req:get_path()` - Get request path

## Response Object

The response object provides:

### Methods

#### `res:render(view, data)`
Renders a template (requires views middleware).

```lua
res:render('user', {
    name = 'John',
    age = 30
})
```

#### `res:json(data)`
Sends JSON response (requires JSON middleware).

```lua
res:json({
    status = 'ok',
    data = { ... }
})
```

#### `res:cookie(name, value[, options])`
Sets a cookie (requires cookie middleware).

```lua
res:cookie('session', token, {
    maxAge = 3600,
    httpOnly = true
})
```

#### `res:status(code[, message])`
Sets response status.

```lua
res:status(404, 'Not Found')
```

#### `res:set_header(name, value)`
Sets response header.

```lua
res:set_header('Content-Type', 'application/json')
```

#### `res:finish([data])`
Ends the response.

```lua
res:finish('Done')
```

## Examples

See examples/app.lua
