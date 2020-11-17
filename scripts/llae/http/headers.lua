local class = require 'llae.class'

local headers = class(nil,'llae.http.headers')

function headers:_init(  )
	self._headers = {}
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

return headers