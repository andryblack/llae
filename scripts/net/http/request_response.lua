local class = require 'llae.class'
local log = require 'llae.log'
local utils = require 'llae.utils'
local archive = require 'archive'



local response = class(require 'net.http.headers','http.request.response')

function response:_init( data  )
	self._headers = data.headers
	self._version = data.version
	self._code = data.code
	self._message = data.message
	self._decoder = (require 'net.http.read_decoder').new(self,data.connection,data.tail)
	self._connection = data.connection

	local encoding = self:get_header('Content-Encoding')
	local transfer_encoding = self:get_header('Transfer-Encoding')
	
	if transfer_encoding and 
		transfer_encoding == 'chunked' then
		self._decoder = (require 'net.http.chunked_decoder').new(self)
	end
	if encoding then
		-- log.debug('use compression',encoding)
		-- for n,v in self:foreach_header() do
		-- 	log.debug('header',n,':',v)
		-- end
		local uncompress
		if encoding == 'gzip' then
			uncompress = archive.new_gunzip_read()
		elseif encoding == 'deflate' then
			uncompress = archive.new_inflate_read(true)
			--log.debug('deflate encoding',self._length)
		else
			error('unsupported encoding ' .. encoding)
		end
		self._decoder = (require 'net.http.uncompress_decoder').new(self,uncompress)
	end

end



function response:get_code(  )
	return self._code
end

function response:get_message(  )
	return self._message
end


function response:read(  )
	if self._closed then
		return nil
	end
	if self._decoder:is_end() then
		--log.debug('stream end')
		self:on_closed()
		self._closed = true
		return nil
	end

	local ch,e = self._decoder:read()
	
	if not ch then
		--log.debug('response end')
		self._error = e
		self:on_closed()
		self._closed = true
	end
	return ch
end

function response:read_body(  )
	local d = {}
	while not self._closed do
		local ch = self:read()
		if not ch then
			break
		end
		--log.debug('read body',#ch)
		table.insert(d,ch)
	end
	return table.concat(d)
end

function response:on_closed(  )
	local connection = self:get_header('Connection')
	if not connection or
		connection == 'close' then
		self:close_connection(true)
	end
end

function response:close_connection( closed )
	if self._connection then
		--log.debug('close_connection')
		if self._uncompress then
			self._uncompress:finish()
			self._uncompress = nil
		end
		if not closed then
			local r,err = self._connection:shutdown()
			if err then
				log.error('shutdown request failed:',err)
			end
		end
		self._connection:close()
		self._connection = nil
	end
end

function response:close(  )
	self:close_connection()
end

return response