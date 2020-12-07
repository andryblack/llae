local class = require 'llae.class'

local http_parser = class(require 'net.http.headers','http.parser')


http_parser.max_method_len = 1024
http_parser.max_header_len = 1024
http_parser.max_start_len = 1024

function http_parser:_init(  )
	self._headers = {}
	self._data = ''
end

function http_parser:read( client )
	local ch,e = client:read()
	assert(not e,e)
	if ch then
		self._data = self._data .. ch
		--print('parser_read',ch)
	end
	return ch
end

function http_parser:parse_header( client )
	while true do
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if line then
			self._data = tail
			if line == '' then
				--print('headers end')
				return true
			end
			local name,value = string.match(line,'^([^:]+)%s*:%s*(.*)$')
			if not name then
				error('failed parse header')
			end
			--print('header:',name,value)
			local t = type(self._headers[name])
			if t == 'nil' then
				self._headers[name] = value
			elseif t == 'table' then
				table.insert(self._headers[name],value)
			else
				self._headers[name] = {self._headers[name],value}
			end
		else
			if #self._data > self.max_header_len then
				error('failed parse headers')
			end
			return false
		end
	end
end

return http_parser