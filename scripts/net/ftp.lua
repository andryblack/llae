local class = require 'llae.class'
local log = require 'llae.log'
local path = require 'llae.path'
local fs = require 'llae.fs'
local async = require 'llae.async'
local uv = require 'uv'


local ftp = class(require 'net.connection','net.fpt')

function ftp:_init( data )
	self._received_cmds = {}
	self._received_data = ''
	self:_configure_connection(data or {})
end

function ftp:_cmd_read( )
	while not self._received_cmds[1] and self._cmd_con do
		local ch,e = self._cmd_con:read()
		assert(not e,e)
		if ch then
			--log.debug('rc',ch,#ch)
			self._received_data = self._received_data .. ch
			while true do
				local pos = string.find(self._received_data,'\r\n',1,true)
				if pos then
					if pos ~= 1 then
						local cmd = string.sub(self._received_data,1,pos-1)
						log.debug('<<<<<',cmd)		
						--log.error(table.concat({string.byte(cmd,1,3)},','))
							
						local cmd_code = string.match(cmd,'^(%d%d%d)')
						if not cmd_code then
							log.error('invalid cmd <'..cmd..'>')
							error('invalid cmd ' .. cmd)
						end
						local data = string.sub(cmd,4)
						table.insert(self._received_cmds,{tonumber(cmd_code),data})
					end
					self._received_data = string.sub(self._received_data,pos+2)
				else
					break
				end
			end
		else
			log.debug('connection closed')
			self._cmd_con = nil
			break
			--print('parser_read',ch)
		end
	end
	return table.remove(self._received_cmds,1)
end

function ftp:_send_cmd( cmd )
	log.debug('>>>>>',cmd)
	assert(self._cmd_con:write(cmd..'\r\n'))
end

function ftp:_expect_status( status , skip)
	while true do
		local cmd = self:_cmd_read()
		if not cmd then
			return false
		end
		if cmd[1] == status then
			return true,cmd[2]
		end
		if not skip or cmd[1] ~= skip then
			log.error('invalid status',cmd[1],status)
			return false
		end
	end
end

function ftp:_cmd_pop(  )
	return table.remove(self._received_cmds,1)
end

function ftp:_drop_all(status)
	while true do
		local cmd = self:_cmd_pop()
		if not cmd then
			return true
		end
		if cmd[1] ~= status then
			log.error('invalid status',cmd[1],status)
			return false
		end
	end
end

local function parse_passv(data)
	local a = {}
	local arg = assert(string.match(data,'%(([%d,]+)%)'))
	for n in string.gmatch(arg,'%d+') do
		table.insert(a,tonumber(n))
	end
	assert( #a > 4 )
	local host = table.concat({a[1],a[2],a[3],a[4]},'.')
	local b = 0
	for i=5,#a do
		b = b * 256	
		b = b | a[i]
	end
	return host,b
end

function ftp:connect( addr, port )
	self._port = port or 21
	self._cmd_con = self:_create_connection()

	log.debug('resolve',addr)
	local err
	self._ip_list,err = uv.getaddrinfo(addr)
	if not self._ip_list then
		return nil,err
	end
	local ip = nil
	for _,v in ipairs(self._ip_list) do
		if v.addr and v.socktype=='tcp' then
			ip = v.addr
			break
		end
	end
	if not ip then
		return nil,'failed resolve ip for ' .. addr
	end
	log.debug('connect to',ip,self._port)
	local res,err = self._cmd_con:connect(ip,self._port)
	if not res then
		log.error('failed connect to',ip,self._port)
		self._connection:close()
		return nil,err
	end
	log.debug('connected')
	if not self:_expect_status(220) then
		return nil,'need 220'
	end
	--self:_send_cmd('FEAT')
	self:_drop_all(220)
	self:_send_cmd('USER anonymous')
	if not self:_expect_status(331,220) then
		return nil,'need 331'
	end
	-- self:_drop_all(220)
	-- self:_drop_all(331)
	
	self:_send_cmd('PASS anonymous@anonymous.com')
	if not self:_expect_status(230) then
		return nil,'need 230'
	end
	
	-- while self._cmd_con do
	-- 	local cmd = self:_cmd_read()
	-- 	log.info('<<<<:',cmd[1],cmd[2])
	-- end
	return true
end

function ftp:getfile( name , to , cb)
	self:_send_cmd('CWD ' .. path.dirname(name))
	if not self:_expect_status(250) then
		error('need 250')
	end

	self:_send_cmd('TYPE I')
	if not self:_expect_status(200) then
		error('need 200')
	end

	
	self:_send_cmd('PASV')
	
	local st,data = self:_expect_status(227)
	
	if not st then
		error('need 227')
	end
	local paddr,pport = parse_passv(data)
	log.debug('open transfer connection',paddr,pport)
	local con = uv.tcp_connection.new()
	assert(con:connect(paddr,pport))

	self:_send_cmd('TYPE I')
	if not self:_expect_status(200) then
		error('need 200')
	end

	local total = 0

	fs.unlink(to)
	local dst = assert(fs.open(to,fs.O_WRONLY|fs.O_CREAT))
	local loaded = 0
	async.run(function()
		while true do
			local ch,err = con:read()
			if not ch then
				if err then
					error(err)
				end
				log.debug('transfer connection closed')
				con:close()
				dst:close()
				break
			end
			dst:write(ch)
			if cb then
				cb(ch,total)
			end
		end
		--p:update(loaded,total)
	end)

	self:_send_cmd('RETR ' .. path.basename(name))
	st,data =  self:_expect_status(150)
	if not st then
		error('need 150')
	end
	total = tonumber(string.match(data,'%((%d+).+%)'))

	if not self:_expect_status(226) then
		error('need 226')
	end
end

function ftp:close(  )
	self:_send_cmd('QUIT')
end

return ftp