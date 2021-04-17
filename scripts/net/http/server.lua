local uv = require 'uv'
local log = require 'llae.log'

-- server

local class = require 'llae.class'

local server = class(nil,'http.server')

server.parser = require 'net.http.server_parser'
server.request = require 'net.http.server_request'
server.response = require 'net.http.server_response'

server.defaults = {
	backlog = 128
}

function server:_init( cb )
	self._server = uv.tcp_server.new()
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
function server:_read_function( client , cb )
	--connection_num = connection_num + 1
	--local self_num = connection_num
	log.debug('start connection')
	while true do
		--print('start load',self_num)
		local p = self.parser.new(cb)
		p.request = self.request
		if p:load(client) then
			break
		end
	end
	log.debug('close')
	client:shutdown()
	client:close()
end

function server:on_connection( err )
	assert(not err, err)
	local client = uv.tcp_connection.new()
	self._server:accept(client)
	local connection_thread = coroutine.create(self._read_function)
	local res,err = coroutine.resume(connection_thread,self,client,function(req) 
		req._client = client
		local resp = self.response.new(client,req)
		self._cb(req,resp)
		return resp
	end)
	if not res then
		error(err)
	end
end



return server
