local json = require 'json'
---The llae.json module provides JSON encoding and decoding functionality with additional features for sorted encoding.
---It extends the core json C module with Lua-specific enhancements.
---@class llae.json : json
local _M = setmetatable({},{__index=json})


---@type fun(gen:json.gen,data:any)
local encode_impl;

encode_impl = function (gen,data)
	local t = type(data)
	if t == 'table' then
		if data[1] or json.is_array(data) then
			gen:array_open()
			for _,v in ipairs(data--[[@as any[] ]]) do
				encode_impl(gen,v)
			end
			gen:array_close()
		else
			gen:map_open()
			---@type string[]
			local keys = {}
			for key,_ in pairs(data--[[@as table<any,any>]]) do
				table.insert(keys,tostring(key))
			end
			table.sort(keys)
			for _,key in ipairs(keys) do
				gen:string(key)
				encode_impl(gen,data[key])
			end
			gen:map_close()
		end
	elseif t == 'number' then
		if math.floor(data) == data then
			gen:integer(data)
		else
			gen:double(data)
		end
	elseif t == 'boolean' then
		gen:bool(data)
	elseif t == 'nil' then
		gen:null()
	else
		gen:string(tostring(data))
	end
end

--- Encodes a Lua value into a JSON string with sorted object keys.
--- Object keys are sorted alphabetically for consistent output.
---@param data any The Lua value to encode (table, string, number, boolean, or nil)
---@param formatted boolean? Whether to format the output with indentation (default: false)
---@return string The JSON string with sorted keys
function _M.encode_sorted(data,formatted)
	local gen = json.gen.new(formatted)
	encode_impl(gen,data)
	local res = gen:get_buffer()
	gen:free()
	return res
end

return _M