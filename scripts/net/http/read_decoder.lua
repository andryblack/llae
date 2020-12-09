local class = require 'llae.class'
local log = require 'llae.log'

local decoder = class(nil,'net.http.read_decoder')


function decoder:_init( resp , connection , data )
	self._length = tonumber(resp:get_header('Content-Length')) 
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
		log.debug('connection stream end')
		return nil
	end
	if self._data then
		local b = self._data
		self._data = nil
		if b ~= '' then
			if self._length then
				self._length = self._length - #b
			end
			log.debug('return headers tail',#b)
			return b
		end
	end

	local ch,e = self._connection:read()
	if ch then
		log.debug('connection readed',#ch)
		if self._length then
			self._length = self._length - #ch
		end
	end
	return ch,e
end
-- 		log.debug('connection readed ',#ch)
-- 		if self._uncompress then
-- 			self:_write_to_uncompress(ch)
-- 			ch,e = self._uncompress:read(false)
-- 			if ch then
-- 				log.debug('uncompress read:',#ch,e)
-- 			else
-- 				log.debug('uncompress read end:',e)
-- 				self._uncompress = nil
-- 			end
-- 		else
-- 			if self._length then
-- 				self._length = self._length - #ch
-- 			end
-- 		end
-- 	elseif self._uncompress then
-- 		self._uncompress:finish()
-- 		ch,e = self._uncompress:read()
-- 		if ch then
-- 			log.debug('uncompress read:',#ch,e)
-- 		else
-- 			log.debug('uncompress read end:',e)
-- 			self._uncompress = nil
-- 		end
-- 	end
-- end

-- function decoder:write( data )
-- 	if data and data ~= '' then
-- 		if self._length then
-- 			self._length = self._length - #data
-- 			if self._length <= 0 then
-- 				local ch = data:sub(1,#data+self._length)
-- 				log.debug('uncompress finish',#data,#ch,self._length)
-- 				self._uncompress:finish(ch)
-- 				if self._length < 0 then
-- 					-- @todo to next requrst
-- 					--self._body = string:sub(self._length)
-- 				end
-- 			else
-- 				self._uncompress:write(data)
-- 			end
-- 		else
-- 			self._uncompress:write(data)
-- 		end
-- 	end
-- end

return decoder