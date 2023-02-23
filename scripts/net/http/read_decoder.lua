local class = require 'llae.class'
local log = require 'llae.log'

local decoder = class(nil,'net.http.read_decoder')


function decoder:_init( resp , connection , data , default_length )
	self._length = tonumber(resp:get_header('Content-Length') or default_length) 
	--log.debug('decoder:',self._length)
	self._connection = connection
	self._data = data
end

function decoder:is_end(  )
	if self._length and self._length <= 0 then
		return true
	end
end

function decoder:read(  )
	if self._length and self._length <= 0 then
		--log.debug('connection stream end')
		return nil
	end
	if self._data then
		local b = self._data
		self._data = nil
		if b ~= '' then
			if self._length then
				self._length = self._length - #b
			end
			--log.debug('return headers tail',#b)
			return b
		end
	end

	--log.debug('connection start read')
	local ch,e = self._connection:read()
	if ch then
		--log.debug('connection readed',#ch)
		if self._length then
			self._length = self._length - #ch
		end
	end
	return ch,e
end

return decoder