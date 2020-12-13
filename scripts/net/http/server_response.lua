local class = require 'llae.class'
local path = require 'llae.path'
local fs = require 'llae.fs'
local archive = require 'archive'
local log = require 'llae.log'
local utils = require 'llae.utils'
local timestamp = require 'net.http.timestamp'


local response = class(require 'net.http.headers','http.server.response')

-- mime
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


local deflate_encoding = class(nil,'http.server.deflate_encoding')
deflate_encoding.encoding = 'deflate'
function deflate_encoding:_init(  )
	self._stream = archive.new_deflate_read()
end
function deflate_encoding:write( ... )
	return self._stream:write(...)
end
function deflate_encoding:read_buffer( )
	return self._stream:read_buffer()
end
function deflate_encoding:finish(...)
	return self._stream:finish(...)
end
function deflate_encoding:send_async( file )
	utils.run(function()
		assert(self._stream:send(file))
	end,true)
end

function response:_init( client , req)
	response.baseclass._init(self)
	self._client = client
	self._req = req
	self._data = {}
	self._code = 200
	local accept = self._req:get_header('Accept-Encoding')
	if accept and string.find(accept,'deflate') then
		self._compress = deflate_encoding.new()
	end
end

local function get_len(d)
	if type(d) == 'string' then
		return #d
	else
		return d:get_len()
	end
end

function response:_send_response( with_data )
	log.debug('response:_send_response',with_data)
	if self._status and (self._status ~= 200) then
		log.debug('reset compression for status',self._status)
		self._compress = nil
	end
	if with_data and not next(self._data) then
		log.debug('reset compression for emppty data')
		self._compress = nil
	end

	if self._compress then
		if self:get_header('Content-Encoding') then
			self._compress = nil
		else
		 	self:set_header('Content-Encoding',self._compress.encoding)
		end 
	end
	
	local send_data = nil 
	if with_data then
		local data_size = 0
		if self._compress then
			send_data = {}
			log.debug('compress data',#self._data)
			self._compress:finish(self._data)
			while true do
				local ch,er = self._compress:read_buffer()
				if er then
					error( er )
				end
				if ch then
					data_size = data_size + ch:get_len()
					table.insert(send_data,ch)
				else
					break
				end
			end
		else
			send_data = self._data
			for _,v in ipairs(self._data) do
				data_size = data_size + get_len(v)
			end
		end
		self._data = nil
		self:set_header('Content-Length',data_size)
	end
	if not self:get_header('Content-Type') then
		self:set_header('Content-Type','text/plain')
	end
	local connection = self._req:get_header('Connection')
	if not self._keep_alive and (not connection or string.lower(connection) ~= 'keep-alive') then
		self:set_header('Connection', 'close')
	elseif not connection or (connection == 'keep-alive') then
		self:set_header('Connection', 'keep-alive')
		self._keep_alive = true
	end
	local r = {
		'HTTP/' .. (self._version or self._req._version or '1.0') ..
			' ' .. (self._code or 200) .. ' ' .. (self._status or 'OK')
	}
	for hn,hv in pairs(self._headers) do
		table.insert(r,hn..': ' .. hv)
	end

	local headers_data = table.concat(r,'\r\n') .. '\r\n\r\n'
	self._headers = nil	
	self._client:write(headers_data)
	if send_data then
		self._client:write(send_data)
	end
end


function response:remove_header( header )
	assert(self._headers,'response already sended')
	self:set_header(header,nil)
end

function response:status( code , status )
	assert(self._headers,'response already sended')
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
function response:_finish( )
	if not self._keep_alive then
		self._closed = true
	else
		if self._client.flush then
			self._client:flush()
		end
	end
	self._client = nil
end
function response:finish( data )
	log.debug('response:finish')
	if data then
		assert(self._data and self._client,'already finished')
		table.insert(self._data,data)
	end
	if not self._client then
		return
	end
	self:_send_response(true)
	self:_finish()
end

function response:is_finished(  )
	return not self._data
end

function response:get_connection(  )
	return self._client
end


local function send_404(resp,path,e) 
	resp:status(404)
	resp:write(e)
	resp:write(path)
	resp:finish()
end

function response:_need_compress_file( ftype, conf )
	if not self._compress then
		return false
	end
	if conf and conf.dnt_compress then
		return false
	end
	return true
end

function response:send_static_file( fpath , conf )
	assert(self._headers,'response already sended')
	assert(not next(self._data),'data not empty')
	log.debug('send_static_file',fpath)
	local stat,e = fs.stat(fpath)
	if not stat then
		send_404(self,fpath,e)
	else
		local req = self._req
		--print('access file ',path,stat.mtim.sec)
		local last = req:get_header('If-Modified-Since')
		if last then
			--print('If-Modified-Since:',last)
			local last_time = timestamp.parse(last)
			if last_time and last_time >= stat.mtim.sec then
				log.debug('file not modified since',last)
				self:status(304)
				return self:finish()
			end
		end
		local f,e = fs.open(fpath,fs.O_RDONLY)
		if not f then
			log.error('faled open',fpath,e)
			send_404(self,fpath,e)
		else
			local ftype = (conf and conf.type) or default_content_type[path.extension(fpath)]
			self:set_header('Content-Type',ftype)
			self:set_header('Last-Modified',timestamp.format(stat.mtim.sec))
			self:set_header('Cache-Control','public,max-age=0')
			if self:_need_compress_file(ftype, conf) then
				self._compress:send_async(f)
				local data_size = 0
				while true do
					local ch,er = self._compress:read_buffer()
					if er then
						error( er )
					end
					if ch then
						table.insert(self._data,ch)
					else
						break
					end
				end
				self:set_header('Content-Encoding',self._compress.encoding)
				self._compress = nil
				self:set_header('Content-Length',data_size)
				self:_send_response(true)
				self:_finish()
			else
				--print('opened',path)
				self:set_header('Content-Length',stat.size)
				self:_send_response(false)
				self._client:send(f)
				self:_finish()
			end
			f:close()
		end
	end
end

return response