local class = require 'llae.class'

local http_parser = require 'net.http.parser'

local parser = class(http_parser,'http.request.parser')

function parser:_init( response )
	parser.baseclass._init(self)
	self.response = response
end


function parser:parse_start( client )
	local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
	if not line then
		if #self._data > self.max_start_len then
			error('failed parse method\n' .. self._data)
		end
		return false
	end
	local ver,code,message = string.match(line,'^HTTP/(%d%.%d)%s(%d+)%s(.+)$')
	if ver then
		self._version = ver
		self._code = code
		self._message = message
		self._data = tail
		return true
	else
		error('failed parse method\n' .. line)
	end
end


function parser:load( client )
	while not self:parse_start(client) do 
		if not self:read(client) then
			return nil,'unexpected end'
		end
	end
	while not self:parse_header(client) do 
		if not self:read(client) then
			return nil,'unexpected end'
		end
	end
	local resp = self.response.new{
		headers = self._headers,
		version = self._version,
		code = tonumber(self._code),
		message = self._message,
		connection = client,
		tail = self._data
	}
	self._data = ''
	return resp
end

return parser