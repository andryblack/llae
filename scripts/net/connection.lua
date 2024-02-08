local class = require 'llae.class'
local log = require 'llae.log'
local url = require 'net.url'
local uv = require 'llae.uv'

local connection = class()

function connection:_create_connection()
	if self._proxy then
		return self._proxy:create()
	end
	return uv.tcp_connection.new()
end

function connection:_configure_connection(data)
	if data.proxy then
		local proxy = url.parse(data.proxy)
		if proxy.scheme ~= 'socks5' then
			error('unsupported proxy ' .. tostring(proxy.scheme))
		end
		log.debug('use socks5 proxy:',proxy.host,proxy.port)
		self._proxy = {
			addr = proxy.host,
			port = tonumber(proxy.port),
			user = proxy.user,
			pass = proxy.password,
			create = function(self)
				local net = require 'net'
				return assert(net.socks5.tcp_connection.new(self.addr,self.port,self.user,self.pass))
			end
		}
	end
end

return connection