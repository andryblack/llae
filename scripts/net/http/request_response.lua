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
	self._connection = data.connection
	self._body = data.tail or ''
	self._length = tonumber(self:get_header('Content-Length')) 
	

	local encoding = self:get_header('Content-Encoding')
	if encoding then
		if encoding == 'gzip' then
			self._uncompress = archive.new_gunzip_read()
			if self._body and self._body ~= '' then
				local d = self._body
				self._body = nil
				self:_write_to_uncompress(d)
			end
		end
	end
end

function response:_write_to_uncompress( data )
	if data and data ~= '' then
		if self._length then
			self._length = self._length - #data
			if self._length <= 0 then
				local ch = data:sub(1,#data+self._length)
				log.debug('uncompress finish',#data,#ch,self._length)
				self._uncompress:finish(ch)
				if self._length < 0 then
					-- @todo to next requrst
					--self._body = string:sub(self._length)
				end
			else
				self._uncompress:write(data)
			end
		else
			self._uncompress:write(data)
		end
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
	if self._length and self._length <= 0 then
		log.debug('stream end')
		self:on_closed()
		self._closed = true
		return nil
	end
	if self._body then
		local b = self._body
		if self._length then
			self._length = self._length - #b
		end
		self._body = nil
		return b
	end
	self._body = nil
	local ch,e = self._connection:read()
	if ch then
		log.debug('connection readed ',#ch)
		if self._uncompress then
			self:_write_to_uncompress(ch)
			ch,e = self._uncompress:read()
			if ch then
				log.debug('uncompress read:',#ch,e)
			else
				log.debug('uncompress read end:',e)
				self._uncompress = nil
			end
		else
			if self._length then
				self._length = self._length - #ch
			end
		end
	elseif self._uncompress then
		ch,e = self._uncompress:read()
		if ch then
			log.debug('uncompress read:',#ch,e)
		else
			log.debug('uncompress read end:',e)
			self._uncompress = nil
		end
	end
	if not ch then
		log.debug('response end')
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
		log.debug('read body',#ch)
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
		log.debug('close_connection')
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