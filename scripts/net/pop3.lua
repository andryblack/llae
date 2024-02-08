local class = require 'llae.class'
local log = require 'llae.log'
local path = require 'llae.path'
local fs = require 'llae.fs'
local async = require 'llae.async'
local url = require 'net.url'
local uv = require 'uv'


local pop3 = class(require 'net.connection','net.pop3')

function pop3:_init(  )
	self._received_cmds = {}
	self._received_data = ''
end

function pop3:_cmd_read( )
	while not self._received_cmds[1] and self._cmd_con do
		--log.debug('read>')
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
						log.debug('<<<<<','|'..cmd..'|')		
						--log.error(table.concat({string.byte(cmd,1,3)},','))
							
						-- local cmd_code = string.match(cmd,'^(%d%d%d)')
						-- if not cmd_code then
						-- 	log.error('invalid cmd <'..cmd..'>')
						-- 	error('invalid cmd ' .. cmd)
						-- end
						-- local data = string.sub(cmd,4)
						table.insert(self._received_cmds,cmd)
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

function pop3:_send_cmd( cmd )
	log.debug('>>>>>',cmd)
	assert(self._cmd_con:write(cmd..'\r\n'))
end


function pop3:_wait_ok()
	while true do
		local cmd = self:_cmd_read()
		if not cmd then
			return false, 'closed'
		end
		if cmd:sub(1,3) == '+OK' then
			return cmd:sub(5)
		end
		if cmd:sub(1,4) == '-ERR' then
			self._error = cmd:sub(6)
			return false, self._error
		end
		log.debug('skip',cmd)
		-- if cmd[1] == status then
		-- 	return true,cmd[2]
		-- end
		-- if not skip or cmd[1] ~= skip then
		-- 	log.error('invalid status',cmd[1],status)
		-- 	return false
		-- end
	end
end

function pop3:_cmd_pop(  )
	return table.remove(self._received_cmds,1)
end

function pop3:_drop_all(status)
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

function pop3:connect( data )

	self._port = data.port or 110
	local addr = data.host

	self:_configure_connection(data)

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

	if data.ssl then
		local http = require 'net.http'
		local ssl = require 'ssl'
		local ctx = http.get_ssl_ctx()
		self._ssl = ssl.connection.new( ctx, self._cmd_con )
		self._cmd_con = self._ssl
		local res,err = self._ssl:configure()
		if not res then
			return res,err
		end
		res,err = self._ssl:set_host( addr )
		if not res then
			return res,err
		end
		log.debug('handshake')
		res,err = self._ssl:handshake()
		if not res then
			return res,err
		end
	end


	--self:_send_cmd('HELLO')
	if not self:_wait_ok() then
		return nil,'need OK, get ERR ' .. (self._error or 'unknown')
	end

	self:_send_cmd('USER ' .. data.user)
	if not self:_wait_ok() then
		return nil,'need OK, get ERR ' .. (self._error or 'unknown')
	end

	self:_send_cmd('PASS ' .. data.pass)
	if not self:_wait_ok() then
		return nil,'need OK, get ERR ' .. (self._error or 'unknown')
	end

	-- --self:_send_cmd('FEAT')
	-- self:_drop_all(220)
	-- self:_send_cmd('USER anonymous')
	-- if not self:_expect_status(331,220) then
	-- 	return nil,'need 331'
	-- end
	-- -- self:_drop_all(220)
	-- -- self:_drop_all(331)
	
	-- self:_send_cmd('PASS anonymous@anonymous.com')
	-- if not self:_expect_status(230) then
	-- 	return nil,'need 230'
	-- end
	
	-- while self._cmd_con do
	-- 	local cmd = self:_cmd_read()
	-- 	log.info('<<<<:',cmd[1],cmd[2])
	-- end
	return true
end

function pop3:get_count()
	self:_send_cmd('STAT')
	local res,err = self:_wait_ok()
	if not res then
		return nil,err
	end
	local count,bytes = string.match(res,'(%d+) (%d+)')
	log.debug('STAT: ','>'..count,bytes)
	return tonumber(count)
end

function pop3:getfile( name , to , cb)
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

function pop3:get_and_delete(mailnum)
	self:_send_cmd('LIST ' .. mailnum)
	local res,err = self:_wait_ok()
	if not res then
		return nil,err
	end
	--local line = self:_cmd_read()
	local num,size = string.match(res,'(%d+) (%d+)')
	self:_send_cmd('RETR ' .. mailnum)
	local res,err = self:_wait_ok()
	if not res then
		return nil,err
	end
	--local size = string.match(res,'(%d+) .*')
	size = tonumber(size)
	log.debug('read',size,'bytes')
	local res = {}
	local rsize = 0
	while true do
		local line = self:_cmd_read()
		if line == '.' then
			break
		else
			rsize = rsize + #line
			table.insert(res,line)
		end
	end
	local data = table.concat( res, '\r\n' )

	self:_send_cmd('DELE ' .. mailnum)
	self:_wait_ok()
	-- while size > 0 do
	-- 	local d = self:_cmd_pop()
	-- 	if not d then
	-- 		break
	-- 	end
	-- 	table.insert(res,d..'\r\n')
	-- 	size = size - #d - 2
	-- end
	-- if #self._received_data > 0 then
	-- 	table.insert(res,self._received_data)
	-- 	size = size - #self._received_data
	-- end
	-- while size > 0 do
	-- 	local ch,e = self._cmd_con:read()
	-- 	assert(not e,e)
	-- 	if ch then
	-- 		table.insert(res,tostring(ch))
	-- 		size = size - #ch
	-- 	end
	-- end
	log.debug('readed:',size,#data,rsize)
	return data
end

function pop3:close(  )
	self:_send_cmd('QUIT')
	self:_wait_ok()
	self._cmd_con:shutdown()
end

return pop3