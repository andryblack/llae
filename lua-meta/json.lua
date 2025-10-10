---@meta json

---Core JSON functionality for encoding and decoding JSON data.
---@class json
local json = {}

---Options for JSON decoding.
---@class json.decode_options
---@field safe boolean? If true returns nil,error_message on failure instead of raising error
---@field mark_arrays boolean? If true marks JSON arrays with special metatable
---@field validate_utf boolean? Enables/disables UTF-8 validation
---@field allow_comments boolean? Enables/disables comment support

--- Decodes a JSON string into Lua values.
---@param data string|llae.buffer_base The JSON string to decode
---@param options json.decode_options|boolean? Table of options or boolean for safe mode
---@param mark_arrays boolean? If true marks JSON arrays with special metatable
---@return any? Decoded Lua value(s) on success
---@return string? Error message if decoding fails
function json.decode(data, options, mark_arrays) end

--- Encodes a Lua value into a JSON string.
--- Order of keys in objects is not guaranteed.
--- Nil values in the middle of arrays are encoded as null.
--- Nil values at the end of arrays are trimmed.
---@param value any The Lua value to encode (table, string, number, boolean, or nil)
---@param beautify boolean? Whether to format the output with indentation (default: false)
---@return string JSON string
function json.encode(value, beautify) end

--- Marks a table as a JSON array.
--- Used to force array encoding for empty or sparse tables.
---@param table table The table to mark as an array
---@return table The same table with array marking metatable
function json.array(table) end

--- Checks if a value is marked as a JSON array.
---@param value any Value to check
---@return boolean True if the value is marked as a JSON array
function json.is_array(value) end

---JSON generator for streaming JSON creation.
---@class json.gen
local gen = {}

--- Creates a new JSON generator.
---@param beautify boolean? Whether to format the output with indentation
---@return json.gen A new JSON generator instance
function gen.new(beautify) end

--- Opens a JSON object.
function gen:map_open() end

--- Closes a JSON object.
function gen:map_close() end

--- Opens a JSON array.
function gen:array_open() end

--- Closes a JSON array.
function gen:array_close() end

--- Adds a string value to the JSON.
---@param value string The string value to add
function gen:string(value) end

--- Adds a null value to the JSON.
function gen:null() end

--- Adds a boolean value to the JSON.
---@param value boolean The boolean value to add
function gen:bool(value) end

--- Adds an integer value to the JSON.
---@param value integer The integer value to add
function gen:integer(value) end

--- Adds a double/float value to the JSON.
---@param value number The number value to add
function gen:double(value) end

--- Gets the generated JSON string.
---@return string The generated JSON string
function gen:get_buffer() end

--- Frees the generator resources.
function gen:free() end

json.gen = gen

return json
