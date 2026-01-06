local log = require 'llae.log'
local uv = require 'llae.uv'
local class = require 'llae.class'

---@class net.dns.plugin
local plugin = class(nil,'net.dns.plugin')

---@param host string
---@return uv.getaddrinfo.item[]?
---@return string?
function plugin:resolve(host)
	return nil,'not implemented'
end

---@class net.dnd.ip_entry

---@class net.dns.cache_entry
---@field time integer
---@field ip_list uv.getaddrinfo.item[]

local dns = {
	---@type table<string,net.dns.cache_entry>
	_resolve_cache = {},
	---@type table<string,uv.getaddrinfo.item[]>
	_override = {},
	---@type net.dns.plugin[]
	_plugins = {},

	plugin = plugin
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
	if not ip_list or not next(ip_list) then
		for i,v in ipairs(dns._plugins) do
			---@type string?
			local perr
			ip_list,perr = v:resolve(host)
			if ip_list and next(ip_list) then
				break
			end
			if perr then
				if not err then
					err = perr
				else
					err = err .. ', ' .. perr
				end
			end
		end
	end

	if not ip_list then
		return nil,err or 'not found'
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

---@param plugin net.dns.plugin
function dns.register_plugin(plugin)
	table.insert(dns._plugins,plugin)
end

---@param plugin net.dns.plugin
function dns.unregister_plugin(plugin)
	for i,v in ipairs(dns._plugins) do
		if v == plugin then
			table.remove(dns._plugins,i)
			break
		end
	end
end

return dns