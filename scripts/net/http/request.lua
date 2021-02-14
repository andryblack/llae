local url = require 'net.url'
local uv = require 'uv'
local ssl = require 'ssl'

local class = require 'llae.class'
local log = require 'llae.log'


local request = class(require 'net.http.headers','http.request')

request.parser = require 'net.http.request_parser'
request.response = require 'net.http.request_response'

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
	self._version = args.version or '1.1'
	self._body = args.body or ''
end

function request:get_path(  )
	local p = self._url.path or ''
	if self._url.query then
		local qstring = tostring(self._url.query)
		if qstring ~= "" then
			p = p .. '?' .. qstring
		end
	end
	return p
end

function request:_connect( port )
	local ip = nil
	for _,v in ipairs(self._ip_list) do
		if v.addr and v.socktype=='tcp' then
			local ip = v.addr
			log.debug('connect to',ip,port)
			self._connection = uv.tcp_connection:new()
			local res,err = self._connection:connect(ip,port)
			if not res then
				log.debug('failed connect to',ip,port,err)
				self._connection:close()
			else
				return res
			end
		end
	end
	return nil,'failed connect to ' .. self._url.host
end

function request:exec(  )
	local err = nil
	log.debug('resolve',self._url.host)
	self._ip_list,err = uv.getaddrinfo(self._url.host)
	if not self._ip_list then
		return nil,err
	end
	local port = self._url.port or url.services[self._url.scheme]
	local res,err = self:_connect(port)
	if not res then
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
	if not self:get_header('Accept-Encoding') then
		table.insert(headers,'Accept-Encoding: deflate, gzip')
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
	local send_data = {
		self._method .. ' ' .. self:get_path() .. ' HTTP/' .. self._version .. '\r\n',
		table.concat(headers,'\r\n'),
		'\r\n\r\n', 
	}
	if self._body and self._body ~= '' then
		table.insert(send_data,self._body)
	end
	local res,err = self._connection:write(send_data)
	if not res then
		return res,err
	end
	local p = self.parser.new(self.response)
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