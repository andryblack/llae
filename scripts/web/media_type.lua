local media_type = {}

---Extracts the media type from a Content-Type header value.
---Parameters (such as `; charset=utf-8`) and surrounding whitespace are ignored,
---and the media type is lowercased.
---@param value string?
---@return string? Media type in lowercase, or nil if the value is missing or empty
function media_type.get( value )
	if type(value) ~= 'string' then
		return nil
	end
	local mt = value:match('^([^;]*)')
	if not mt then
		return nil
	end
	mt = mt:lower():gsub('^%s+',''):gsub('%s+$','')
	if mt == '' then
		return nil
	end
	return mt
end

---Checks whether a Content-Type header value denotes the expected media type.
---@param value string?
---@param expected string Media type to compare with, in lowercase
---@return boolean
function media_type.is( value, expected )
	return media_type.get(value) == expected
end

return media_type
