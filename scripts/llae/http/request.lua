local url = require 'net.url'
local uv = require 'uv'
local ssl = require 'ssl'

local http_parser = require 'llae.http.parser'
local class = require 'llae.class'
local log = require 'llae.log'

local parser = class(http_parser,'http.request.parser')

function parser:_init( )
	parser.baseclass._init(self)
end


function parser:parse_start( client )
	local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
	if not line then
		if #self._data > self.max_start_len then
			error('failed parse method\n' .. self._data)
		end
		return false
	end
	local ver,code,message = string.match(line,'^HTTP/(%d%.%d)%s(%d+)%s(.+)$')
	if ver then
		self._version = ver
		self._code = code
		self._message = message
		self._data = tail
		return true
	else
		error('failed parse method\n' .. line)
	end
end

local response = class(require 'llae.http.headers','http.request.response')

function response:_init( data )
	self._headers = data.headers
	self._version = data.version
	self._code = data.code
	self._message = data.message
	self._connection = data.connection
end

function response:get_code(  )
	return self._code
end

function response:get_message(  )
	return self._message
end


function response:read(  )
	if self._length and self._length <= 0 then
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
		if self._length then
			self._length = self._length - #ch
		end
	else
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
		table.insert(d,ch)
	end
	return table.concat(d)
end

function response:on_closed(  )
	local connection = self:get_header('Connection')
	if not connection or
		connection == 'close' then
		self:close_connection()
	end
end

function response:close_connection( )
	if self._connection then
		log.debug('close_connection')
		local r,err = self._connection:shutdown()
		if err then
			log.error('shutdown request failed:',err)
		end
		self._connection:close()
		self._connection = nil
	end
end

function response:close(  )
	self:close_connection()
end

function parser:load( client )
	while not self:parse_start(client) do 
		if not self:read(client) then
			return nil,'unexpected end'
		end
	end
	while not self:parse_header(client) do 
		if not self:read(client) then
			return nil,'unexpected end'
		end
	end
	local resp = response.new{
		headers = self._headers,
		version = self._version,
		code = tonumber(self._code),
		message = self._message,
		connection = client
	}
	resp._length = tonumber(self:get_header('Content-Length')) 
	if self._data and self._data ~= '' then
		resp._body = self._data
		self._data = ''
	end
	return resp
end

local request = class(require 'llae.http.headers','http.request')

function request.get_ssl_ctx() 
	if not request._ssl_ctx then
		request._ssl_ctx = ssl.ctx:new()
		assert(request._ssl_ctx:init())
	end
	return request._ssl_ctx
end

function request:_init( args )
	assert(args.url,'need url')
	request.baseclass._init(self,args.headers or {})
	log.debug('new request to',args.url)
	local comp = url.parse(args.url)
	self._url = comp
	self._method = args.method or 'GET'
	self._version = args.version or '1.0'
	self._body = args.body or ''
end


function request:exec(  )
	self._connection = uv.tcp_connection:new()
	local err = nil
	log.debug('resolve',self._url.host)
	self._ip_list,err = uv.getaddrinfo(self._url.host)
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
		return nil,'failed resolve ip for ' .. self._url.host
	end
	local port = self._url.port or url.services[self._url.scheme]
	log.debug('connect to',ip,port)
	local res,err = self._connection:connect(ip,port)
	if not res then
		log.error('failed connect to',ip,port)
		self._connection:close()
		return nil,err
	end
	log.debug('connected')

	self._headers['Content-Length'] = #self._body
	if not self:get_header('Connection') then
		self._headers['Connection'] = 'close'
	end

	local headers = {}
	if not self:get_header('Host') then
		table.insert(headers,'Host: ' .. self._url.host)
	end
	for k,v in pairs(self._headers) do
		table.insert(headers,k..': ' .. v)
	end

	if self._url.scheme == 'https' then
		log.debug('open ssl connection')
		self._tcp = self._connection
		self._ssl = ssl.connection:new( request.get_ssl_ctx(), self._connection)
		self._connection = self._ssl
		local res,err = self._ssl:configure()
		if not res then
			return res,err
		end
		res,err = self._ssl:set_host(self._url.host)
		if not res then
			return res,err
		end
		log.debug('handshake')
		res,err = self._ssl:handshake()
		if not res then
			return res,err
		end
		log.debug('handshake success')
	end
	-- print(self._method .. ' ' .. (self._url.path or '/') .. ' HTTP/' .. self._version .. '\r\n',
	-- 	table.concat(headers,'\r\n'),
	-- 	'\r\n\r\n', 
	-- 	self._body)
	self._connection:write{
		self._method .. ' ' .. (self._url.path or '/') .. ' HTTP/' .. self._version .. '\r\n',
		table.concat(headers,'\r\n'),
		'\r\n\r\n', 
		self._body
	}
	local p = parser.new()
	while true do
		local resp,err = p:load(self._connection) 
		if resp then
			if resp:get_code() == 302 or resp:get_code() == 301 then
				resp:close()
				local redirect_url = resp:get_header('Location')
				log.debug('redirect to',redirect_url)
				self._url = url.parse(redirect_url)
				return self:exec()
			end
		end
		return resp,err
	end
end

return request