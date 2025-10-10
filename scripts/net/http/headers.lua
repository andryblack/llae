local class = require 'llae.class'
local log = require 'llae.log'

---@class net.http.headers
---@field new fun(init:table<string,string|string[]>?) : net.http.headers
---@field private _headers table<string,string|string[]>
---@field private _list string[]
local headers = class(nil,'http.headers')

---@param init table<string,string|string[]>?
function headers:_init( init )
	self._headers = {}
	self._list = {}
	if init then
		local names = {}
		for name,value in pairs(init) do
			table.insert(names,name)
		end
		table.sort(names)
		for _,name in ipairs(names) do
			self:set_header(name,init[name])
		end
	end
end

function headers:get_header( name )
	local h = self._headers[name]
	if h then
		return h
	end
	local n = string.lower(name)
	for hn,hv in pairs(self._headers) do
		if string.lower(hn) == n then
			return hv
		end
	end
	return nil
end

function headers:set_header( name , value )
	local n = string.lower(name)
	for hn,hv in pairs(self._headers) do
		if string.lower(hn) == n then
			self._headers[hn] = value
			return
		end
	end
	self._headers[name] = value
	table.insert(self._list,name)
end

function headers:add_header( name, value )
	local n = string.lower(name)
	for hn,hv in pairs(self._headers) do
		if string.lower(hn) == n then
			local current = self._headers[hn]
			if type(current) ~= 'table' then
				self._headers[hn] = {current,value}
			else
				table.insert(current,value)
			end
			return
		end
	end
	self._headers[name] = {value}
	table.insert(self._list,name)
end


function headers:foreach_header( )
	return pairs(self._headers)
end

---@protected
function headers:_dump_headers()
	for hn,hv in pairs(self._headers) do
		if type(hv) == 'table' then
			for _,v in ipairs(hv) do
				log.debug('header',hn,v)
			end
		else
			log.debug('header',hn,hv)
		end
	end
end

---@protected
function headers:_write_headers(r)
	for _,hn in ipairs(self._list) do
		local hv = self._headers[hn]
		if hv then
			if type(hv) == 'table' then
				for _,v in ipairs(hv) do
					table.insert(r,hn..': ' .. v)
				end
			else
				table.insert(r,hn..': ' .. hv)
			end
		end
	end
end

return headers