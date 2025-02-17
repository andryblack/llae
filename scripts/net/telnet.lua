local class = require 'llae.class'
local uv = require 'llae.uv'
local log = require 'llae.log'

local telnet = class()

function telnet:_init(host,port)
	self._port = port or error('need port')
	self._host = host or error('need host')
end

function telnet:connect()
	if self._connected then
		return true
	end
	local err
	self._cmd_con = uv.tcp_connection.new()
	self._ip_list,err = uv.getaddrinfo(self._host)
	if not self._ip_list then
		return nil,'resolve failed: ' .. tostring(err)
	end
	local connected = false
	local errors = {}
	for _,v in ipairs(self._ip_list) do
		if v.addr and v.socktype=='tcp' then
			local ip = v.addr
			log.debug('connect to',ip,self._port)
			local res,err = self._cmd_con:connect(ip,self._port)
			if res then
				connected = true
				break
			else
				table.insert(errors,err)
			end
		end
	end
	if not connected then
		return nil,table.concat(errors,',')
	end
	self._connected = true
	self._rc_lines = {}
	self._received_data = ''
	return self:wait_ok()
end

function telnet:close()
	self._connected = false
	if self._cmd_con then
		self._cmd_con:shutdown()
	end
end

function telnet:send(msg)
	if not self._cmd_con then
		return nil,'closed'
	end
	if not self._connected then
		return nil, 'not connected'
	end
	return self._cmd_con:write(msg .. '\r\n')
end

function telnet:read_line()
	while self._cmd_con do
		local l = table.remove(self._rc_lines,1)
		if l then
			return l
		end
		local ch,e = self._cmd_con:read()
		if e then
			return nil,e
		end
		if ch then
			self._received_data = self._received_data .. ch
			while true do
				local pos = string.find(self._received_data,'\r\n',1,true)
				if pos then
					if pos ~= 1 then
						local cmd = string.sub(self._received_data,1,pos-1)
						log.debug('<','['..cmd..']')
						table.insert(self._rc_lines,cmd)
					end
					self._received_data = string.sub(self._received_data,pos+2)
				else
					break
				end
			end
		else
			log.debug('connection closed')
			self._cmd_con = nil
		end
	end
end

function telnet:wait_ok(func)
	local res
	while true do
		local line,err = self:read_line()
		if not line then
			return nil,err
		end
		if line == 'OK' then
			return res or true
		elseif line:sub(1,3) == 'KO:' then
			return false, line:sub(5)
		elseif func then
			res = func(line) or res
			--log.info('skip line',line)
		end
	end
end

function telnet:send_expect_ok(msg,func)
	log.debug('>','['..msg..']')
	local res,err = self:send(msg)
	if not res then
		return nil,err
	end
	return self:wait_ok(func)
end

return telnet