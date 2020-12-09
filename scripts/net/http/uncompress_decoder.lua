local class = require 'llae.class'
local log = require 'llae.log'
local decoder = class(nil,'net.http.uncompress_decoder')


function decoder:_init( resp , uncompress )
	self._upstream = resp._decoder
	self._uncompress = uncompress
	self._total = 0
end

function decoder:is_end(  )
	return not self._uncompress
end

function decoder:read( )
	if not self._uncompress then
		return nil
	end

	local ch,e = self._upstream:read()
	if ch then
		if #ch > 0 then
			log.debug('write to uncompress',#ch)
			self._uncompress:write(ch)
		end
	elseif not self._finished then
		log.debug('finish uncompress')
		self._uncompress:finish()
		self._finished = true
	end
	if not ch and e then
		return ch,e 
	end

	ch,e = self._uncompress:read(not not self._finished)
	if not ch then
		log.debug('uncompress finished read',e,self._total)
		self._uncompress = nil
	else
		log.debug('readed from uncompress:',#ch)
		self._total = self._total + #ch
	end
	return ch,e
end

return decoder