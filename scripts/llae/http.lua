local llae = require 'llae'
local url = require 'net.url'
local fs = require 'llae.fs'
local uv = require 'uv'

local http = {}


local http_parser = {}

local max_method_len = 1024
local max_header_len = 1024
local max_start_len = 1024

function http_parser:read( client )
	local ch,e = client:read()
	assert(not e,e)
	if ch then
		self._data = self._data .. ch
		--print('parser_read',ch)
	end
	return ch
end

function http_parser:parse_header( client )
	while true do
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if line then
			self._data = tail
			if line == '' then
				--print('headers end')
				return true
			end
			local name,value = string.match(line,'^([^:]+)%s*:%s*(.*)$')
			if not name then
				error('failed parse header')
			end
			--print('header:',name,value)
			local t = type(self._headers[name])
			if t == 'nil' then
				self._headers[name] = value
			elseif t == 'table' then
				table.insert(self._headers[name],value)
			else
				self._headers[name] = {self._headers[name],value}
			end
		else
			if #self._data > max_header_len then
				error('failed parse headers')
			end
			return false
		end
	end
end


do
	-- server

	local request = {}

	local request_mt = {
		__index = request
	}

	function request.new( method, path, headers , version, length)
		local res = {
			_method = method,
			_path = path,
			_headers = headers,
			_version = version,
			_length = length
		}
		
		return setmetatable(res,request_mt)
	end

	function request:get_path(  )
		return self._path
	end

	function request:get_method( )
		return self._method
	end

	function request:get_header( name )
		return self._headers[name]
	end

	function request:on_closed(  )
		-- body
	end
	function request:read(  )
		if self._length <= 0 then
			return nil
		end
		if self._body then
			local b = self._body
			self._length = self._length - #b
			self._body = nil
			return b
		end
		self._body = nil
		local ch,e = self._client:read()
		assert(not e,e)
		if ch then
			self._length = self._length - #ch
		else
			self:on_closed()
			self._closed = true
		end
		return ch
	end

	function request:read_body( )
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

	local parser = { read = http_parser.read , parse_header = http_parser.parse_header }
	local parser_mt = {
		__index = parser
	}

	

	function parser.new( cb  )
		local r = {
			_data = '',
			_headers = {},
			_cb = cb,
		}
		return setmetatable(r,parser_mt)
	end

	

	function parser:load( client )
		while not self:parse_method(client) do 
			if not self:read(client) then
				if self._data == '' then
					return true
				end
				error('unexpected end')
			end
		end
		while not self:parse_header(client) do 
			if not self:read(client) then
				error('unexpected end')
			end
		end
		local _length = tonumber(self._headers['Content-Length']) or 0
		local req = request.new(self._method,self._path,self._headers,self._ver,_length)
		if self._data and self._data ~= '' then
			req._body = self._data
			self._data = ''
		end
		--print('call handler')
		self._cb(req)
		while req:read() do
		end
		return req._closed
	end

	function parser:parse_method( client )
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if not line then
			if #self._data > max_method_len then
				error('failed parse method\n' .. self._data)
			end
			return false
		end
		local method,path,ver = string.match(line,'^([^%s]+)%s([^%s]+)%sHTTP/(%d%.%d)$')
		if method then
			self._method = method
			self._path = path
			self._ver = ver
			self._data = tail
			--print('found method',method)
			return true
		else
			error('failed parse method\n' .. line)
		end
	end

	


	local response = {}

	local response_mt = {
		__index = response
	}

	function response.new( client , req)
		local res = {
			_client = client,
			_headers = {},
			_headers_map = {},
			_req = req,
			_data = {}
		}
		return setmetatable(res,response_mt)
	end

	local function check_flush_headers( self )
		if self._headers then
			if not self._dnt_process_headers then
				--print('write headers')
				if not self._headers_map['Content-Length'] then
					local size = 0
					for _,v in ipairs(self._data) do
						size = size + #v
					end
					self:set_header('Content-Length',size)
				end
				if not self._headers_map['Content-Type'] then
					self:set_header('Content-Type','text/plain')
				end
				local connection = self._req:get_header('Connection')
				if not self._keep_alive and (not connection or string.lower(connection) ~= 'keep-alive') then
					self:set_header('Connection', 'close')
				elseif not connection or (connection == 'keep-alive') then
					self:set_header('Connection', 'keep-alive')
					self._keep_alive = true
				end
			end
			local r = {
				'HTTP/' .. (self._version or self._req._version or '1.0') ..
					' ' .. (self._code or 200) .. ' ' .. (self._status or 'OK')
			}
			for _,v in ipairs(self._headers) do
				table.insert(r,v[1]..': ' .. v[2])
			end
			table.insert(self._data,1,table.concat(r,'\r\n'))
			table.insert(self._data,2,'\r\n\r\n')
			self._headers = nil
		end
	end

	function response:flush(  )
		check_flush_headers(self)
		--print('write data')
		if self._data and next(self._data) then
			self._client:write(self._data)
		end
		self._data = nil
	end

	function response:set_header( header, value )
		assert(self._headers,'header already sended')
		table.insert(self._headers,{header,value})
		self._headers_map[header] = value
	end

	function response:send_reply( code , msg)
		assert(self._headers,'header already sended')
		self._code = code
		if msg then
			self:write(msg)
		end
	end

	function response:status( code )
		self._code = code
	end

	function response:write( data )
		assert(self._data,'already finished')
		table.insert(self._data,data)
	end

	function response:keep_alive(  )
		self._keep_alive = true
	end
	function response:set_version( ver )
		self._version = ver
	end
	function response:dnt_process_headers(  )
		self._dnt_process_headers = true
	end
	function response:finish( data )
		
		if data then
			assert(self._data,'already finished')
			table.insert(self._data,data)
		end
		self:flush()
		if not self._keep_alive then
			--print('shutdown')
			self._client:shutdown()
		end
	end

	function response:get_connection(  )
		return self._client
	end

	local months_s = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
	}
	local months = {}
	for i,n in ipairs(months_s) do
		months[n]=i
	end
	local function send_404(resp,path,e) 
		resp:status(404)
		resp:write(e)
		resp:write(path)
		resp:finish()
	end

	function response:send_static_file( path )
		local stat,e = fs.stat(path)
		if not stat then
			send_404(self,path,e)
		else
			local req = self._req
			--print('access file ',path,stat.mtim.sec)
			local last = req:get_header('If-Modified-Since')
			if last then
				--print('If-Modified-Since:',last)
				local dn,d,mn,y,h,m,s = string.match(last,'^(...), (%d%d) (...) (%d%d%d%d) (%d%d):(%d%d):(%d%d) GMT$')
				if dn then
					local last_time = os.time{
						year = tonumber(y),
						month = months[mn],
						day = tonumber(d),
						hour = tonumber(h),
						min = tonumber(m),
						sec = tonumber(s)
					}
					--print('If-Modified-Since:',last_time)
					if last_time >= stat.mtim.sec then
						self:status(304)
						self:finish()
						return
					end
				end
			end
			local f,e = fs.open(path)
			if not f then
				print('faled open',path,e)
				send_404(self,path,e)
			else
				--print('opened',path)
				self:set_header('Content-Length',stat.size)
				self:set_header('Last-Modified',os.date('%a, %d %b %Y %H:%M:%S GMT',stat.mtim.sec))
				self:set_header('Cache-Control','public,max-age=0')
				self:flush()
				self._client:send(f)
				self:finish()
			end
		end
	end

	local server = {}

	server.defaults = {
		backlog = 128
	}

	local server_mt = {
		__index = server
	}

	function server.new( cb )
		local r = {}
		r._server = uv.tcp_server:new()
		r._backlog = server.defaults.backlog
		r._cb = cb
		return setmetatable(r,server_mt)
	end

	function server:listen( port, addr )
		local res,err = self._server:bind(addr,port)
		if not res then return nil,err end
		return self._server:listen(self._backlog,function(err)
			self:on_connection(err)
		end)
	end

	function server:stop(  )
		self._server:close()
	end

	--local connection_num = 0
	local function read_function( client , cb )
		--connection_num = connection_num + 1
		--local self_num = connection_num
		--print('start connection',self_num)
		while true do
			--print('start load',self_num)
			local p = parser.new(cb)
			if p:load(client) then
				break
			end
		end
		--print('close')
		client:close()
	end

	function server:on_connection( err )
		assert(not err, err)
		local client = uv.tcp_connection:new()
		self._server:accept(client)
		local connection_thread = coroutine.create(read_function)
		local res,err = coroutine.resume(connection_thread,client,function(req) 
			req._client = client
			self._cb(req,response.new(client,req))
		end)
		if not res then
			error(err)
		end
		-- client:start_read( read_function, client ,
		-- 	function( req )
		-- 		req._client = client
		-- 		self._cb(req,response.new(client,req))
		-- 	end) 
		-- local resp = response.new(client)
		-- request.start(client)
		-- self._cb( req , resp )
	end


	
	function server:send_static_file( req, resp, path )
		resp:send_static_file(path)
	end

	http.server = server

end

function http.get_tls_ctx(  )
	if not http._tls_ctx then
		http._tls_ctx = llae.newTLSCtx()
	end
	return http._tls_ctx
end

do -- client

	local parser = { read = http_parser.read , parse_header = http_parser.parse_header }
	local parser_mt = {
		__index = parser
	}

	

	function parser.new( )
		local r = {
			_data = '',
			_headers = {}
		}
		return setmetatable(r,parser_mt)
	end
	

	function parser:parse_start( client )
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if not line then
			if #self._data > max_start_len then
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
			--print('found method',method)
			return true
		else
			error('failed parse method\n' .. line)
		end
	end


	local response = {}

	local response_mt = {
		__index = response
	}


	function response.new( res )
		return setmetatable(res,response_mt)
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
		if not self._headers.Connection or
			self._headers.Connection == 'close' then
			self:close_connection()
		end
	end

	function response:close_connection( )
		if self._connection then
			print('close_connection')
			self._connection:close()
			self._connection = nil
		end
	end

	function response:close(  )
		if self._connection then
			self._connection:shutdown()
		end
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
			_headers = self._headers,
			_version = self._version,
			_code = tonumber(self._code),
			_message = self._message,
			_connection = client
		}
		resp._length = tonumber(self._headers['Content-Length']) 
		if self._data and self._data ~= '' then
			resp._body = self._data
			self._data = ''
		end
		return resp
	end

	local request = {}

	local request_mt = {
		__index = request
	}

	function request.new( args )
		assert(args.url,'need url')
		--print('new request to',args.url)
		local comp = url.parse(args.url)
		local res = {
			_url = comp,
			_method = args.method or 'GET',
			_headers = args.headers or {},
			_version = args.version or '1.0',
			_body = args.body or '',
		}
		
		return setmetatable(res,request_mt)
	end


	function request:exec(  )
		self._connection = uv.tcp_connection:new()
		local err = nil
		--print('resolve',self._url.host)
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
		--print('connect to',ip,port)
		local res,err = self._connection:connect(ip,port)
		if not res then
			return nil,err
		end

		self._headers['Content-Length'] = #self._body
		if not self._headers['Connection'] then
			self._headers['Connection'] = 'close'
		end

		local headers = {}
		for k,v in pairs(self._headers) do
			table.insert(headers,k..': ' .. v)
		end

		if self._url.scheme == 'https' then
			self._tcp = self._connection
			self._tls = llae.newTLS(http.get_tls_ctx(),self._connection)
			self._connection = self._tls

			local res,err = self._tls:handshake()
			if not res then
				return res,err
			end
		end

		self._connection:write{
			self._method .. ' ' .. (self._url.path or '/') .. ' HTTP/' .. self._version .. '\r\n',
			table.concat(headers,'\r\n'),
			'\r\n\r\n', 
			self._body
		}
		local p = parser.new()
		return p:load(self._connection) 
	end

	http.request = request
end

function http.createRequest( args )
	return http.request.new(args)
end


function http.createServer( cb )
	return http.server.new(cb)
end

return http