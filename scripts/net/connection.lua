local class = require 'llae.class'
local log = require 'llae.log'
local url = require 'net.url'
local uv = require 'llae.uv'
local dns = require 'net.dns'

---@class net.proxy
---@field scheme string
---@field addr string
---@field port integer?
---@field user string?
---@field pass string?
---@field create fun(self:net.proxy):uv.tcp_connection

---@class net.connection
---@field _proxy net.proxy?
---@field _remote_dns boolean? resolve target host on the proxy side
local connection = class(nil,'net.connection')

---@public
function connection:_create_connection()
	if self._proxy then
		return self._proxy:create()
	end
	return uv.tcp_connection.new()
end

--- Configure target connection proxy.
--- `proxy` is a socks5:// URL, `remote_dns` selects who resolves the target host:
--- false (default) - locally, true - on the proxy side.
---@public
---@param data {proxy:string?,remote_dns:boolean?}
function connection:_configure_connection(data)
	self._remote_dns = data.remote_dns == true
	if not data.proxy then
		return
	end
	local proxy = url.parse(data.proxy)
	if proxy.scheme ~= 'socks5' then
		error('unsupported proxy ' .. tostring(proxy.scheme))
	end
	log.debug('use socks5 proxy:',proxy.host,proxy.port,
		self._remote_dns and '(resolve host on proxy)' or '(resolve host locally)')
	self._proxy = {
		scheme = proxy.scheme,
		addr = proxy.host,
		port = tonumber(proxy.port),
		user = proxy.user,
		pass = proxy.password,
		create = function(self)
			local socks5 = require 'net.socks5'
			return assert(socks5.tcp_connection.new(self.addr,self.port,self.user,self.pass))
		end
	}
end

---@public
---@return boolean
function connection:_resolve_on_proxy()
	return self._proxy ~= nil and self._remote_dns == true
end

--- Resolve connection target host into a list of connect addresses.
--- When the proxy resolves hosts itself, the host itself is returned as
--- the connect address, so the proxy gets a domain name (SOCKS5 ATYP 0x03).
--- Explicit dns overrides still win: they only exist to redirect traffic locally.
---@public
---@param host string
---@return uv.getaddrinfo.item[]?
---@return string?
function connection:_resolve_host(host)
	if self:_resolve_on_proxy() then
		local over = dns.get_override(host)
		if over then
			return over
		end
		log.debug('resolve on proxy side',host)
		return {{ addr = host, socktype = 'tcp' }}
	end
	return dns.resolve(host)
end

return connection
