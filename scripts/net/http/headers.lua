local class = require 'llae.class'

local headers = class(nil,'http.headers')

function headers:_init( init )
	self._headers = init or {}
end

function headers:get_header( name )
	local h = self._headers[name]
	if h then
		return h
	end
	local n = string.lower(name)
	for hn,hv in pairs(self._headers) do
		if string.lower(hn) == n then
			return hv
		end
	end
	return nil
end

function headers:set_header( name , value )
	local n = string.lower(name)
	for hn,hv in pairs(self._headers) do
		if string.lower(hn) == n then
			self._headers[hn] = value
			return
		end
	end
	self._headers[name] = value
end

return headers