
local class = require 'llae.class'

local parser = class(require 'net.http.parser','http.server.parser')


function parser:_init( cb  )
	parser.baseclass._init(self)
	self._cb = cb
end

function parser:load( client )
	while not self:parse_method(client) do 
		if not self:read(client) then
			if self._data == '' then
				return true
			end
			error('unexpected end')
		end
	end
	while not self:parse_header(client) do 
		if not self:read(client) then
			error('unexpected end')
		end
	end
	local _length = tonumber(self:get_header('Content-Length') or 0)
	local req = self.request.new(self._method,self._protocol,self._path,self._headers,self._ver,_length)
	if self._data and self._data ~= '' then
		req._body = self._data
		self._data = ''
	end
	--print('call handler')
	local resp = self._cb(req)
	if resp._closed then
		return true
	end
	while req:read() do
	end
	return req._closed
end

function parser:parse_method( client )
	local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
	if not line then
		if #self._data > self.max_method_len then
			error('failed parse method\n' .. self._data)
		end
		return false
	end
	local method,path,protocol,ver = string.match(line,'^(%u+)%s([^%s]+)%s(%u+)/(%d%.%d)$')
	if method then
		self._method = method
		self._path = path
		self._ver = ver
		self._data = tail
		self._protocol = protocol
		--print('found method',method)
		return true
	else
		error('failed parse method\n' .. line)
	end
end

return parser