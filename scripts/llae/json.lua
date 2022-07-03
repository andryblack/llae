local json = require 'json'
local _M = setmetatable({},{__index=json})


local encode_impl;

encode_impl = function (gen,data)
	local t = type(data)
	if t == 'table' then
		if data[1] or json.is_array(data) then
			gen:array_open()
			for i,v in ipairs(data) do
				encode_impl(gen,v)
			end
			gen:array_close()
		else
			gen:map_open()
			local keys = {}
			for key,_ in pairs(data) do
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

function _M.encode_sorted(data)
	local gen = json.gen.new()
	encode_impl(gen,data)
	local res = gen:get_buffer()
	gen:free()
	return res
end

return _M