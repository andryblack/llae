local log = require 'llae.log'
local uv = require 'llae.uv'

local dns = {
	_resolve_cache = {},
	_override = {}
}


function dns.resolve( host )

	local cached = dns._resolve_cache[host]
	local now = os.time()
	if cached and (os.difftime(now,cached.time) < 30) then
		return cached.ip_list
	end
	
	if dns._override[host] then
		log.debug('resolve override',host)
		return dns._override[host]
	end
	log.debug('resolve',host)
	local ip_list,err = uv.getaddrinfo(host)
	if not ip_list then
		return nil,err
	end
	if next(ip_list) then
		dns._resolve_cache[host] = {
			time = now,
			ip_list = ip_list
		}
	end
	return ip_list
end

function dns.override(host,addr)
	local over = dns._override[host] or {}
	dns._override[host] = over
	table.insert(over,addr)
end

return dns