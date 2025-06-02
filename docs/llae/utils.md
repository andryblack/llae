# Utils Module

The `utils` module provides a collection of utility functions for table manipulation, environment variable handling, argument parsing, and more.

## Table Functions

### `utils.clear_table(t)`
Clears all key-value pairs from a table.

**Parameters:**
- `t` (table): The table to clear

**Example:**
```lua
local utils = require 'llae.utils'
local t = {1, 2, 3, a = "test"}
utils.clear_table(t)
-- t is now {}
```

### `utils.merge(...)`
Merges multiple tables into a new table. Later tables override values from earlier tables.

**Parameters:**
- `...` (tables): Variable number of tables to merge

**Returns:**
- (table): A new table containing all key-value pairs from the input tables

**Example:**
```lua
local utils = require 'llae.utils'
local t1 = {a = 1, b = 2}
local t2 = {b = 3, c = 4}
local result = utils.merge(t1, t2)
-- result is {a = 1, b = 3, c = 4}
```

### `utils.list_concat(a, b)`
Concatenates two array-like tables.

**Parameters:**
- `a` (table): First array-like table
- `b` (table): Second array-like table

**Returns:**
- (table): A new table containing all elements from both tables in order

**Example:**
```lua
local utils = require 'llae.utils'
local t1 = {1, 2, 3}
local t2 = {4, 5, 6}
local result = utils.list_concat(t1, t2)
-- result is {1, 2, 3, 4, 5, 6}
```

### `utils.reversedipairs(t)`
Iterator that traverses an array-like table in reverse order.

**Parameters:**
- `t` (table): The table to iterate over

**Returns:**
- Iterator function that returns index-value pairs in reverse order

**Example:**
```lua
local utils = require 'llae.utils'
local t = {1, 2, 3, 4, 5}
for i, v in utils.reversedipairs(t) do
    print(i, v)
end
-- Prints:
-- 5 5
-- 4 4
-- 3 3
-- 2 2
-- 1 1
```

## String Functions

### `utils.replace_env(str)`
Replaces environment variable references in a string with their values. If a variable doesn't exist, the reference is left unchanged.

**Parameters:**
- `str` (string): String containing environment variable references in the format `${VAR_NAME}`

**Returns:**
- (string): String with environment variables replaced with their values, or original reference if variable not found

**Example:**
```lua
local utils = require 'llae.utils'
-- Assuming HOME is set to "/home/user"
local path = utils.replace_env("Path: ${HOME}/docs")
-- path is "Path: /home/user/docs"

-- Non-existent variable
local result = utils.replace_env("${NON_EXISTENT}")
-- result is "${NON_EXISTENT}"
```

### `utils.replace_tokens(text, tokens)`
Replaces token references in a string with values from a token table. If a token doesn't exist, the reference is left unchanged.

**Parameters:**
- `text` (string): String containing token references in the format `${TOKEN_NAME}`
- `tokens` (table): Table mapping token names to their values

**Returns:**
- (string): String with tokens replaced with their values, or original reference if token not found

**Example:**
```lua
local utils = require 'llae.utils'
local tokens = {name = "John", age = "30"}
local text = utils.replace_tokens("Name: ${name}, Age: ${age}", tokens)
-- text is "Name: John, Age: 30"

-- Non-existent token
local result = utils.replace_tokens("${unknown}", tokens)
-- result is "${unknown}"
```

## Command Line Arguments

### `utils.parse_args(args)`
Parses command line arguments into a structured table.

**Parameters:**
- `args` (table): Array of command line arguments

**Returns:**
- (table): Table containing:
  - Numbered arguments in array part
  - Named options in dictionary part
  - Original command in index 0

**Features:**
- Supports `--key=value` format for named options
- Supports `--flag` format for boolean flags
- Preserves non-option arguments in order

**Example:**
```lua
local utils = require 'llae.utils'
local args = {
    [0] = "script.lua",
    "--verbose",
    "input.txt",
    "--output=result.txt",
    "extra"
}
local parsed = utils.parse_args(args)
-- parsed is {
--   [0] = "script.lua",
--   [1] = "input.txt",
--   [2] = "extra",
--   verbose = true,
--   output = "result.txt"
-- }
``` 