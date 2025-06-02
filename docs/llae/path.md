# Path Module

The `path` module provides utilities for working with file system paths in a cross-platform manner, supporting both Unix and Windows path formats.

## Functions

### `path.normalize(p)`
Normalizes path separators to forward slashes.

**Parameters:**
- `p` (string): The path to normalize

**Returns:**
- (string): Path with all backslashes and multiple slashes converted to single forward slashes

**Example:**
```lua
local path = require 'llae.path'
print(path.normalize('C:\\Windows\\System32')) -- prints "C:/Windows/System32"
print(path.normalize('path//to//file')) -- prints "path/to/file"
```

### `path.isabsolute(p)`
Checks if a path is absolute.

**Parameters:**
- `p` (string): The path to check

**Returns:**
- (boolean): `true` if the path is absolute, `false` otherwise

**Notes:**
- Recognizes Unix absolute paths (starting with '/')
- Recognizes Windows absolute paths:
  - Drive letter paths (e.g., 'C:/', 'D:\\')
  - UNC paths (starting with '\\')

**Example:**
```lua
local path = require 'llae.path'
print(path.isabsolute('/usr/local')) -- true
print(path.isabsolute('C:/Windows')) -- true
print(path.isabsolute('relative/path')) -- false
```

### `path.join(...)`
Joins path segments with forward slashes.

**Parameters:**
- `...` (strings): Path segments to join

**Returns:**
- (string): Combined path with segments joined by forward slashes

**Example:**
```lua
local path = require 'llae.path'
print(path.join('usr', 'local', 'bin')) -- prints "usr/local/bin"
print(path.join('C:', 'Windows', 'System32')) -- prints "C:/Windows/System32"
```

### `path.basename(path)`
Gets the last portion of a path.

**Parameters:**
- `path` (string): The path to process

**Returns:**
- (string): The last portion of the path (file or directory name)

**Example:**
```lua
local path = require 'llae.path'
print(path.basename('/usr/local/file.txt')) -- prints "file.txt"
print(path.basename('C:\\Windows\\System32')) -- prints "System32"
```

### `path.dirname(path)`
Gets the directory name of a path.

**Parameters:**
- `path` (string): The path to process

**Returns:**
- (string): The directory portion of the path
- Returns empty string for relative paths with no directory component
- Returns '/' for root directory

**Example:**
```lua
local path = require 'llae.path'
print(path.dirname('/usr/local/file.txt')) -- prints "/usr/local"
print(path.dirname('file.txt')) -- prints ""
print(path.dirname('/')) -- prints "/"
```

### `path.extension(path)`
Gets the file extension from a path.

**Parameters:**
- `path` (string): The path to process

**Returns:**
- (string|nil): The file extension without the dot, or nil if there is no extension

**Example:**
```lua
local path = require 'llae.path'
print(path.extension('file.txt')) -- prints "txt"
print(path.extension('script.lua')) -- prints "lua"
print(path.extension('README')) -- prints nil
```

### `path.getabsolute(fn)`
Converts a relative path to an absolute path.

**Parameters:**
- `fn` (string): The path to convert

**Returns:**
- (string): Absolute path
- Returns the input unchanged if it's already absolute

**Example:**
```lua
local path = require 'llae.path'
print(path.getabsolute('relative/path')) -- prints "/current/working/dir/relative/path"
print(path.getabsolute('/absolute/path')) -- prints "/absolute/path"
```

### `path.getrelative(fn)`
Attempts to convert an absolute path to a path relative to the current working directory.

**Parameters:**
- `fn` (string): The path to convert

**Returns:**
- (string): Path relative to current working directory if possible, otherwise returns the original path

**Example:**
```lua
local path = require 'llae.path'
-- Assuming current working directory is /home/user
print(path.getrelative('/home/user/project')) -- prints "project"
print(path.getrelative('/different/path')) -- prints "/different/path"
```

### `path.remove_leading_dirs(fn, count)`
Removes a specified number of leading directory components from a path.

**Parameters:**
- `fn` (string): The path to process
- `count` (number): Number of directory components to remove

**Returns:**
- (string|nil): Path with leading directories removed, or nil if there aren't enough components

**Example:**
```lua
local path = require 'llae.path'
print(path.remove_leading_dirs('a/b/c/d', 2)) -- prints "c/d"
print(path.remove_leading_dirs('a/b', 3)) -- prints nil
```
