# Filesystem Module

The `fs` module provides a comprehensive set of functions for working with the file system. It wraps the libuv filesystem operations and adds additional high-level functionality.

## Core Functions

### `fs.home()`
Returns the user's home directory path.

**Returns:**
- (string): Path to the user's home directory

**Example:**
```lua
local fs = require 'llae.fs'
print(fs.home()) -- prints something like "/home/user" or "C:\Users\user"
```

### `fs.pwd()` / `fs.cwd()`
Gets the current working directory.

**Returns:**
- (string): Path to the current working directory

**Example:**
```lua
local fs = require 'llae.fs'
print(fs.pwd()) -- prints the current working directory
```

### `fs.chdir(path)`
Changes the current working directory.

**Parameters:**
- `path` (string): The directory to change to

**Returns:**
- (boolean): `true` if successful, `false` otherwise

**Example:**
```lua
local fs = require 'llae.fs'
fs.chdir('/tmp')
```

### `fs.exepath()`
Gets the path to the current executable.

**Returns:**
- (string): Path to the current executable

**Example:**
```lua
local fs = require 'llae.fs'
print(fs.exepath()) -- prints path to the current executable
```

## File and Directory Operations

### `fs.isfile(fn)`
Checks if a path points to a regular file.

**Parameters:**
- `fn` (string): Path to check

**Returns:**
- (boolean): `true` if the path points to a regular file, `false` otherwise (including when the path doesn't exist)

**Example:**
```lua
local fs = require 'llae.fs'
print(fs.isfile('example.txt')) -- prints true if example.txt is a regular file
print(fs.isfile('nonexistent')) -- prints false
print(fs.isfile('directory')) -- prints false
```

### `fs.isdir(fn)`
Checks if a path points to a directory.

**Parameters:**
- `fn` (string): Path to check

**Returns:**
- (boolean): `true` if the path points to a directory, `false` otherwise (including when the path doesn't exist)

**Example:**
```lua
local fs = require 'llae.fs'
print(fs.isdir('mydir')) -- prints true if mydir is a directory
print(fs.isdir('nonexistent')) -- prints false
print(fs.isdir('regular_file.txt')) -- prints false
```

### `fs.rmdir_r(dir)`
Recursively removes a directory and all its contents.

**Parameters:**
- `dir` (string): Path to the directory to remove

**Returns:**
- (boolean, string): `true` if successful, or `false` and error message if failed

**Example:**
```lua
local fs = require 'llae.fs'
local ok, err = fs.rmdir_r('old_directory')
if not ok then print('Error:', err) end
```

### `fs.mkdir_r(dir)`
Creates a directory and all necessary parent directories.

**Parameters:**
- `dir` (string): Path to the directory to create

**Returns:**
- (boolean, string): `true` if successful, or `false` and error message if failed

**Example:**
```lua
local fs = require 'llae.fs'
local ok, err = fs.mkdir_r('path/to/new/directory')
if not ok then print('Error:', err) end
```

## File Reading and Writing

### `fs.load_file(fn)`
Reads an entire file into memory.

**Parameters:**
- `fn` (string): Path to the file to read

**Returns:**
- (string): The entire contents of the file

**Example:**
```lua
local fs = require 'llae.fs'
local content = fs.load_file('example.txt')
print(content)
```

### `fs.read_file(fn)`
Creates an iterator to read a file in chunks.

**Parameters:**
- `fn` (string): Path to the file to read

**Returns:**
- (function): Iterator function that returns chunks of the file
- (userdata): File handle

**Example:**
```lua
local fs = require 'llae.fs'
for chunk in fs.read_file('large_file.txt') do
  print(chunk)
end
```

### `fs.write_file(fn, ...)`
Writes data to a file, creating it if it doesn't exist or overwriting it if it does.

**Parameters:**
- `fn` (string): Path to the file to write
- `...` (string): One or more strings to write to the file

**Example:**
```lua
local fs = require 'llae.fs'
fs.write_file('output.txt', 'Hello', ' ', 'World')
```

## Directory Scanning

### `fs.scanfiles_r(dir)`
Recursively scans a directory and returns a list of all files.

**Parameters:**
- `dir` (string): Directory to scan

**Returns:**
- (table): Array of file paths relative to the scanned directory
- (string): Error message if operation failed

**Example:**
```lua
local fs = require 'llae.fs'
local files = fs.scanfiles_r('project_dir')
for _, file in ipairs(files) do
  print(file)
end
```

## Executable Finding

### `fs.find_exe(bin)`
Finds the full path to an executable by searching in PATH.

**Parameters:**
- `bin` (string): Name or path of the executable to find

**Returns:**
- (string): Full path to the executable
- Throws an error if the executable is not found

**Example:**
```lua
local fs = require 'llae.fs'
local python_path = fs.find_exe('python')
print(python_path)
```

## Low-Level File Operations

### `fs.open(filename, mode)`
Opens a file with the specified mode.

**Parameters:**
- `filename` (string): Path to the file to open
- `mode` (number): File open mode (use fs.O_* constants)

**Returns:**
- (userdata, string): File handle and error message if failed

**Example:**
```lua
local fs = require 'llae.fs'
local file = fs.open('example.txt', fs.O_RDONLY)
```

### `fs.open_write(fn)`
Opens a file for writing, creating it if it doesn't exist.

**Parameters:**
- `fn` (string): Path to the file to open

**Returns:**
- (userdata, string): File handle and error message if failed

**Example:**
```lua
local fs = require 'llae.fs'
local file = fs.open_write('output.txt')
file:write('Hello World')
file:close()
``` 