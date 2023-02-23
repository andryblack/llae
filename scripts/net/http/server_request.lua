local class = require 'llae.class'
local archive = require 'archive'
local log = require 'llae.log'

local request = class(require 'net.http.headers','http.server.request')


function request:_init( data )
	request.baseclass._init(self,data.headers)
	self._method = data.method
	self._protocol = data.protocol
	self._path = data.path
	self._version = data.version
	self._client = data.client
	local encoding = self:get_header('Content-Encoding')
	--log.debug('request:')
	--self:_dump_headers()
	self._decoder = (require 'net.http.read_decoder').new(self,self._client,data.data,0)
	if encoding then
		local uncompress
		if encoding == 'gzip' then
			--print('create gzip')
			uncompress = archive.new_gunzip_read()
		elseif encoding == 'deflate' then
			--print('create deflate')
			uncompress = archive.new_inflate_read(true)
			--log.debug('deflate encoding',self._length)
		else
			error('unsupported encoding ' .. encoding)
		end
		self._decoder = (require 'net.http.uncompress_decoder').new(self,uncompress,data.data)
	end
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

function request:getpeername( )
	return self._client:getpeername()
end

function request:on_closed(  )
	-- body
end
function request:read(  )
	if self._closed then
		return nil
	end
	if self._decoder:is_end() then
		--log.debug('stream end')
		self:on_closed()
		self._closed = true
		return nil
	end

	--log.debug('decoder start read')
	local ch,e = self._decoder:read()
	--log.debug('decoder end read')
	
	if not ch then
		--log.debug('request end')
		self._error = e
		self:on_closed()
		self._closed = true
	end
	return ch
end

function request:read_body( )
	if not self._body then
		local d = {}
		while not self._closed do
			--log.debug('read body')
			local ch = self:read()
			if not ch then
				break
			end
			--log.debug('body readed:',#ch)
			table.insert(d,ch)
		end
		self._body = table.concat(d,'')
		--log.debug('total body readed:',#self._body)
	end
	return self._body
end

function request:close(  )
	self._closed = true
end

return request