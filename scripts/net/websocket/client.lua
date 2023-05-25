local http = require 'net.http'
local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'llae.uv'
local log = require 'llae.log'

local client = class(require 'net.websocket.protocol')

client.counter = 0

function client:_init(args)
	assert(args.url,'need url')
	client.baseclass._init(self,true)
	self._args = args
	self._protocol = {'chat','superchat'}
	self._version = 13
end

function client:start( handler )
	self._handler = handler
	local comp = url.parse(self._args.url)
	if comp.scheme == 'ws' then
		comp.scheme = 'http'
	elseif comp.scheme == 'wss' then
		comp.scheme = 'https'
	else
		return nil,'invalid scheme ' .. tostring(comp.scheme)
	end
	client.counter = client.counter + 1
	local ws_key = string.pack('I4I4I4I4',math.random(1,0xffffffff),math.random(1,0xffffffff),math.random(1,0xffffffff),client.counter)
	ws_key = tostring(uv.buffer.base64_encode(ws_key))
	local http_url = comp:build()
	local http_args = setmetatable({
		url = http_url,
		method = 'GET',
		headers = {
			['Connection'] = 'upgrade',
			['Upgrade'] = 'websocket',
			['Sec-WebSocket-Key'] = ws_key,
			['Sec-WebSocket-Protocol'] = table.concat( self._protocol, ", " ),
			['Sec-WebSocket-Version'] = self._version
		}
	},{__index=self._args})
	local http_requiest = http.createRequest(http_args)
	local http_resp,err = http_requiest:exec()
	if not http_resp then
		return nil,err
	end

	if http_resp:get_code() ~= 101 then
		return nil,'invalid response code: ' .. tostring(http_resp:get_code())
	end

	if http_resp:get_header('Upgrade') ~= 'websocket' then
		return nil,'invalid upgrade'
	end

	local sec_resp = http_resp:get_header('Sec-WebSocket-Accept')
	if not sec_resp then
		return nil,'invalid accept'
	end
	--@todo check sec_resp

	self._connection = http_resp._connection
	
	self:_process_rc(http_resp._decoder._data or '')
	while not self._closed and not self._error do
		self:_process_read()
	end
	return not self._error, self._error
end

function client:_write_packet(p)
	return self._connection:write(p)
end


function client:_process_read()
	local ch,err = self._connection:read()
	if ch then
		self:_process_rc(ch)
	elseif not err then
		self._closed = true
	else
		self._error = err
	end
end


return client