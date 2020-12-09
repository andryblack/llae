local class = require 'llae.class'
local log = require 'llae.log'

local decoder = class(nil,'net.http.uncompress_decoder')


function decoder:_init( resp  )
	self._upstream = resp._decoder
	self._len = nil
	self._data = ''
end

function decoder:is_end(  )
	return self._finished
end


function decoder:read( )
	if self._finished then
		return nil
	end

	while not self._len do
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if line then
			self._len = tonumber(line,16)
			if not self._len then
				self._finished = true
				return nil,'invlid chunk size "' .. line .. '"'
			end
			self._data = tail
			log.debug('begin chunk:',self._len,line,#tail)
			break
		else
			local ch,e = self._upstream:read()
			if not ch then
				self._finished = true
				return ch,e
			end
			self._data = self._data .. ch
		end
	end

	if #self._data == 0 then
		local ch,e = self._upstream:read()
		if not ch then
			self._finished = true
			return ch,e
		end
		self._data = ch
	end
	local l = math.min(#self._data,self._len)
	local ch = string.sub(self._data,1,l)
	self._data = string.sub(self._data,l+1)
	self._len = self._len - #ch
	if self._len <= 0 then
		self._len = nil
		log.debug('end chunk')
		while #self._data < 2 do
			local ch,e = self._upstream:read()
			if not ch then
				log.debug('connection end on read chunk tail')
				self._finished = true
				return ch,e
			end
			self._data = self._data .. ch
		end
		local rn = string.sub(self._data,1,2)
		self._data = string.sub(self._data,3)
		if rn ~= '\r\n' then
			self._finished = true
			return nil,'invalid chunk tail'
		end
	end
	log.debug('readed from chunk',#ch)
	return ch
end

return decoder