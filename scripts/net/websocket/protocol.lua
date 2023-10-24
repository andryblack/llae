local class = require 'llae.class'
local log = require 'llae.log'
local uv = require 'llae.uv'
local async = require 'llae.async'

local protocol = class()

protocol.opcode = {
	cont    = 0,
	text    = 1,
	binary  = 2,
	rsv3    = 3,
	rsv4    = 4,
	rsv5    = 5,
	rsv6    = 6,
	rsv7    = 7,
	close   = 8,
	ping    = 9,
	pong    = 10,
	crsvb   = 11,
	crsvc   = 12,
	crsvd   = 13,
	crsve   = 14,
	crsvf   = 15
}

function protocol:_init(mask)
	self._rc_data = ''
	self._rc_fragments = {}
	self._mask = mask
	self._wr_lock = async.lock.new()
end

function protocol:_handle_msg(opcode)
	local payload = table.concat(self._rc_fragments,'')
	self._rc_fragments = {}
	--log.info(string.format('RX: msg %02x',opcode),payload)
	if opcode == protocol.opcode.text then
		self._handler:on_text(payload)
	elseif opcode == protocol.opcode.binary then
		self._handler:on_binary(payload)
	elseif opcode == protocol.opcode.close then
		local reason = string.unpack('>I2',payload)
		self:_close()
		self._handler:on_close(opcode)
	end
end

function protocol:_process_rc(data)
	self._rc_data = self._rc_data .. data
	
	while true do
		local rc_len = #self._rc_data
		if rc_len < 2 then
			return
		end
		local h1,h2 = string.unpack('<I1I1',self._rc_data,1)
		local FIN = h1 & 0x80
		local opcode = h1 & 0x0f
		local mask = h2 & 0x80
		local length = h2 & 0x7f
		local extra = 0
		
		if length == 126 then
			if rc_len < (2+2) then
				return
			end
			extra = 2
			length = string.unpack('>I2',self._rc_data,3)
		elseif length == 127 then
			if (rc_len < (2+8)) then
				return
			end
			extra = 8
			length = string.unpack('>I8',self._rc_data,3)
		end
		if mask ~= 0 then
			extra = extra + 4
		end
		if rc_len < (length + 2 + extra) then
			return
		end
		if length ~= 0 then
			local start = 1+2+extra
			local payload_end = start+length
			local payload = string.sub(self._rc_data,start,payload_end-1)
			self._rc_data = string.sub(self._rc_data,payload_end)
			table.insert(self._rc_fragments,payload)
		end
		if opcode ~= protocol.opcode.cont then
			self._rc_opcode = opcode
		end
		if FIN then
			self:_handle_msg(self._rc_opcode)
		end
	end
end

function protocol:binary(data)
	return self:write_msg(data,true,protocol.opcode.binary)
end

function protocol:text(data)
	return self:write_msg(data,true,protocol.opcode.text)
end

function protocol:close()
	local res, err = self:write_msg(string.pack('<I2',0),true,protocol.opcode.close)
	self:_close()
	return res,err
end

function protocol:write_msg(data,fin,opcode)
	self._wr_lock:lock()
	if self._mask then
		if not self._mask_data or 
			self._mask >= #self._mask_data then
				self._mask_data = uv.random(1024)
			self._mask = 1
		end
	end
	log.info('write_msg',data,fin,opcode)
	local res = {'..'}
	local len = #data
	if #data > 125 then
		table.insert(res,string.pack('>I2',#data))
		len = 126
	end
	

	local h1 = (fin and 0x80 or 0x00) | opcode
	local h2 = len

	if self._mask then
		h2 = h2 | 0x80
		local mask_data = string.unpack('<I4',self._mask_data,self._mask)
		table.insert(res,string.pack('<I4',mask_data))
		self._mask = self._mask + 4
		local rdata = {}
		local i = 1
		while (i+3) <= #data do
			local d = string.unpack('<I4',data,i)
			i = i + 4
			d = d ~ mask_data
			table.insert(rdata,string.pack('<I4',d))
		end
		local j = 0
		while i <= #data do
			local d = string.unpack('<I1',data,i)
			d = d ~ ((mask_data >> (j*8)) & 0xff)
			table.insert(rdata,string.pack('<I1',d))
			i = i + 1
			j = j + 1
		end
		data = table.concat( rdata, '')
	end
	res[1] = string.pack('<I1I1',h1,h2)
	table.insert(res,data)
	local res,err =  self:_write_packet(res)
	self._wr_lock:unlock()
	return res,err
end

return protocol