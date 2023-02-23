local class = require 'llae.class'
local log = require 'llae.log'

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

function headers:foreach_header( )
	return pairs(self._headers)
end

function headers:_dump_headers()
	for hn,hv in pairs(self._headers) do
		if type(hv) == 'table' then
			for _,v in ipairs(hv) do
				log.debug('header',hn,v)
			end
		else
			log.debug('header',hn,hv)
		end
	end
end

function headers:_write_headers(r)
	for hn,hv in pairs(self._headers) do
		if type(hv) == 'table' then
			for _,v in ipairs(hv) do
				table.insert(r,hn..': ' .. v)
			end
		else
			table.insert(r,hn..': ' .. hv)
		end
	end
end

return headers