local uv = require 'uv'
local log = require 'llae.log'
local async = require 'llae.async'

-- server

local class = require 'llae.class'

---HTTP server implementation supporting both HTTP and HTTPS protocols.
---@class net.http.server
---@field new fun(cb:function) : net.http.server
local server = class(nil,'net.http.server')

server.parser = require 'net.http.server_parser'
server.request = require 'net.http.server_request'
server.response = require 'net.http.server_response'

server.defaults = {
	backlog = 128
}

function server:_init( cb  )
	self._server = uv.tcp_server.new()
	self._cb = cb
	self.active_connections = 0
end

--- Starts the server listening for connections.
---@param port integer TCP port to listen on
---@param addr string? IP address to bind to (default: '127.0.0.1')
---@param backlog integer? Connection backlog size (default: 128)
---@return boolean? True if successful
---@return string? Error message if failed
function server:listen( port, addr , backlog )
	local res,err = self._server:bind(addr or '127.0.0.1',port)
	if not res then return nil,err end
	async.run(function()
		while true do
			local res,err = self._server:listen(backlog or server.defaults.backlog)
			if not res then
				if err then
					log.error('failed listen',err)
				end
				break
			end
			self:on_connection()
		end
	end)
	return true
end

--- Stops the server and closes all connections.
function server:stop(  )
	self._server:stop()
end

--local connection_num = 0
function server:_read_function( client , cb )
	--connection_num = connection_num + 1
	--local self_num = connection_num
	--log.debug('start connection')
	self.active_connections = self.active_connections + 1
	while true do
		--print('start load',self_num)
		local p = self.parser.new(cb)
		p.request = self.request
		if p:load(client) then
			break
		end
	end
	self.active_connections = self.active_connections - 1
	--log.debug('close')
	client:shutdown()
	client:close()
end

function server:on_connection( )
	local client = uv.tcp_connection.new()
	local res,err = self._server:accept(client)
	if not res then
		log.error('failed accept',err)
		error(err)
	end

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
