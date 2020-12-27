local class = require 'llae.class'

local request = class(require 'net.http.headers','http.server.request')


function request:_init( method, protocol, path, headers , version, length)
	request.baseclass._init(self,headers)
	self._method = method
	self._protocol = protocol
	self._path = path
	self._headers = headers
	self._version = version
	self._length = length
end

function request:get_path(  )
	return self._path
end

function request:get_protocol( )
	return self._protocol
end

function request:get_method( )
	return self._method
end

function request:on_closed(  )
	-- body
end
function request:read(  )
	if self._length <= 0 then
		return nil
	end
	if self._body then
		local b = self._body
		self._length = self._length - #b
		self._body = nil
		return b
	end
	self._body = nil
	local ch,e = self._client:read()
	assert(not e,e)
	if ch then
		self._length = self._length - #ch
	else
		self:on_closed()
		self._closed = true
	end
	return ch
end

function request:read_body( )
	local d = {}
	while not self._closed do
		local ch = self:read()
		if not ch then
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d)
end

function request:close(  )
	self._closed = true
end

return request