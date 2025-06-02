# JSON Module

The `llae.json` module provides JSON encoding and decoding functionality with additional features for sorted encoding. It extends the core `json` C module with Lua-specific enhancements.

## Functions

### encode(data [, formatted])
Inherited from core `json` module. Encodes a Lua value into a JSON string.

- **Parameters:**
  - `data`: The Lua value to encode (table, string, number, boolean, or nil)
  - `formatted` (optional): Boolean indicating whether to format the output with indentation (default: false)
- **Returns:** JSON string
- **Note:** Order of keys in objects is not guaranteed
- **Array Behavior:**
  - nil values in the middle of arrays are encoded as null
  - nil values at the end of arrays are trimmed
  - Example: `{1, nil, 2}` becomes `[1,null,2]`
  - Example: `{1, 2, nil}` becomes `[1,2]`

### encode_sorted(data [, formatted])
Encodes a Lua value into a JSON string with sorted object keys.

- **Parameters:**
  - `data`: The Lua value to encode (table, string, number, boolean, or nil)
  - `formatted` (optional): Boolean indicating whether to format the output with indentation (default: false)
- **Returns:** JSON string
- **Note:** Object keys are sorted alphabetically for consistent output

### decode(json_string [, options])
Inherited from core `json` module. Decodes a JSON string into Lua values.

- **Parameters:**
  - `json_string`: The JSON string to decode
  - `options` (optional): Table of options:
    - `safe`: Boolean, if true returns nil,error_message on failure instead of raising error
    - `mark_arrays`: Boolean, if true marks JSON arrays with special metatable
    - `validate_utf`: Boolean, enables/disables UTF-8 validation
    - `allow_comments`: Boolean, enables/disables comment support
- **Returns:** Decoded Lua value(s)

### array(table)
Inherited from core `json` module. Marks a table as a JSON array.

- **Parameters:**
  - `table`: The table to mark as an array
- **Returns:** The same table with array marking metatable
- **Note:** Used to force array encoding for empty or sparse tables

### is_array(value)
Inherited from core `json` module. Checks if a value is marked as a JSON array.

- **Parameters:**
  - `value`: Value to check
- **Returns:** Boolean indicating if the value is marked as a JSON array

## Examples

```lua
local json = require 'llae.json'

-- Basic encoding
local data = {
  name = "test",
  values = {1, 2, 3}
}
print(json.encode(data))
-- {"name":"test","values":[1,2,3]}

-- Sorted encoding
print(json.encode_sorted(data))
-- {"name":"test","values":[1,2,3]}

-- Formatted output
print(json.encode_sorted(data, true))
-- {
--   "name": "test",
--   "values": [1, 2, 3]
-- }

-- Working with arrays
local empty_array = json.array{}
print(json.encode(empty_array))
-- []

-- Decoding
local decoded = json.decode('{"name":"test","values":[1,2,3]}')
print(decoded.name) -- "test"
print(decoded.values[1]) -- 1
```
