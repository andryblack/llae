local uv = require 'uv'
local fs = require 'llae.fs'
local path = require 'llae.path'
-- server

local http_parser = require 'llae.http.parser'
local class = require 'llae.class'

local request = class(nil,'http.server.request')

local default_content_type = {
	['css'] = 'text/css',
	['html'] = 'text/html',
	['js'] = 'text/javascript',
	['png'] = 'image/png',
	['svg'] = 'image/svg+xml',
	['jpeg'] = 'image/jpeg',
	['jpg'] = 'image/jpeg',
	['wasm'] ='application/wasm',
}


function request:_init( method, path, headers , version, length)
	self._method = method
	self._path = path
	self._headers = headers
	self._version = version
	self._length = length
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


local parser = class(http_parser,'http.server.parser')


function parser:_init( cb  )
	parser.baseclass._init(self)
	self._cb = cb
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
		if #self._data > self.max_method_len then
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


local response = class(nil,'http.server.response')

function response:_init( client , req)
	self._client = client
	self._headers = {}
	self._headers_map = {}
	self._req = req
	self._data = {}
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

function response:_flush(  )
	check_flush_headers(self)
	--print('write data')
	if self._data and next(self._data) then
		self._client:write(self._data)
	end
	self._data = nil
end

function response:set_header( header, value )
	assert(self._headers,'header already sended')
	if not value then
		return
	end
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

function response:status( code , status )
	self._code = code
	self._status = status
	return self
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
	self:_flush()
	if not self._keep_alive then
		--print('shutdown')
		self._client:shutdown()
	end
end

function response:is_finished(  )
	return not self._data
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

function response:send_static_file( fpath , conf )
	local stat,e = fs.stat(fpath)
	if not stat then
		send_404(self,fpath,e)
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
		local f,e = fs.open(fpath)
		if not f then
			log.error('faled open',fpath,e)
			send_404(self,fpath,e)
		else
			--print('opened',path)
			self:set_header('Content-Length',stat.size)
			self:set_header('Last-Modified',os.date('%a, %d %b %Y %H:%M:%S GMT',stat.mtim.sec))
			self:set_header('Cache-Control','public,max-age=0')
			self:set_header('Content-Type',(conf and conf.type) or default_content_type[path.extension(fpath)])
			self:_flush()
			self._client:send(f)
			self:finish()
		end
	end
end

local server = class(nil,'http.server')

server.defaults = {
	backlog = 128
}

function server:_init( cb )
	self._server = uv.tcp_server:new()
	self._backlog = server.defaults.backlog
	self._cb = cb
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

return server
