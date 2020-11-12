local class = require 'llae.class'
local fs = require 'llae.fs'
local log = require 'llae.log'

local router = class(nil,'router')

local methods = {'get','post'}

local _wildcard_byte = string.byte('*',1)
local _colon_byte = string.byte(':',1)
local _wildcard = {}
local _handler = {}
local _token = {}

local function match_one_path(node, path, f)
	for token in path:gmatch("[^/.]+") do
		local b1 = token:byte(1)
		if _wildcard_byte == b1 then
			local w = {handler=f,token=token:sub(2)}

			if not node[_wildcard] then
				node[_wildcard] = {w}
			else
				table.insert(node[_wildcard],w)
			end
			return
		elseif _colon_byte == b1 then -- if match the ":", store the param_name in "TOKEN" array.
			node[_token] = node[_token] or {}
			token = token:sub(2)
			node = node[_token]
		end
		node[token] = node[token] or {}
		node = node[token]
	end
	node[_handler] = f
end

local function resolve(path, node, params, handler)
	log.debug('resolve begin for',path)
	
	local _, _, current_token, rest = path:find("([^/.]+)(.*)")
	if not current_token then 
		log.debug('resolve end')
		local f = node[_handler]
		if f then
			if handler(f,params) then
				return true
			end
		end
		return false
	end

	local n = node[current_token]
	if n then
		log.debug('resolve continue for',current_token)
		if resolve(rest, n, params, handler) then
			return true
		end
	end
	
	for token, child_node in pairs(node[_token] or {}) do
		local value = params[token]
		params[token] = current_token or value -- store the value in params, resolve tail path
		if resolve(rest,child_node,params,handler) then
			return true
		end
		params[token] = value -- reset the params table.
	end

	for _,wildcard in ipairs(node[_wildcard] or {}) do
		local token = wildcard.token
		log.debug('check wildcard',token)
		local value = params[token]
		params[token] = current_token .. rest
		if handler(wildcard.handler,params) then
			return true
		end
		params[token] = value
	end

	return false
end


function router:_init( ... )
	self._tree = {}
end

function router:resolve(method, path, handler)
	local node   = self._tree[method]
	if not node then 
  		return false
  	end
  	return resolve(path, node, {}, handler)
end

function router:match(method,path,func)
	if type(method) == 'string' then -- always make the method to table.
		method = {method}
	end

	local parsed_methods = {}
	for k, v in pairs(method) do
		if type(v) == 'string' then
			-- convert shorthand methods into longhand
			if parsed_methods[v] == nil then 
				parsed_methods[v] = {} 
			end
			parsed_methods[v][path] = func
		else
			-- pass methods already in longhand format onwards
			parsed_methods[k] = v
		end
	end

	for m, routes in pairs(parsed_methods) do
		for p, f in pairs(routes) do
			if not self._tree[m] then 
				self._tree[m] = {} 
			end
			match_one_path(self._tree[m], p, f)
		end
	end
end

for _,method in ipairs(methods) do
	router[method] = function(self,path,func)
		return self:match(method:upper(),path,func)
	end
end

return router