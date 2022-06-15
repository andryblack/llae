local class = require 'llae.class'
local log = require 'llae.log'
local format = require 'net.http.timestamp'

local cookie = class(require 'web.middle','cookie')

function cookie.parse(str)
	local res = {}
	local pos = 0
	while true do
		local npos = string.find(str,'; ',pos,true)
		local substr = string.sub(str,pos,npos and (npos-1))
		log.debug('substr:',substr)
		local k,v = string.match(substr,'(.+)=(.+)')
		if k and v then
			res[k]=v
			log.debug('set cookie',k,v)
		end
		if not npos then
			break
		end
		pos = npos + 3
	end
	return res
end


function cookie.build(values)
	local kv = {}
	for k,v in pairs(values) do
		table.insert(kv,k .. '=' .. v)
	end
	table.sort(kv)
	return table.concat(kv,'; ')
end

function cookie:handle_request(req,res)
	local cook = req:get_header('Cookie')
	if cook then
		req._cookies = cookie.parse(cook)
	end

	function req:get_cookie(name)
		return self._cookies and self._cookies[name]
	end

	function res:set_cookie(name,data_)
		local data = type(data_) == 'table' and data_ or {value=data_}
		local cooks = self._cookies or {}
		self._cookies = cooks
		local value = data.value
		if data.SameSite then
			value = value .. '; SameSite=' .. data.SameSite
		end
		if data.Secure then
			value = value .. '; Secure'
		end
		if data.HttpOnly then
			value = value .. '; HttpOnly'
		end
		if data.Domain then
			value = value .. '; Domain=' .. data.Domain
		end
		if data.Path then
			value = value .. '; Path=' .. data.Path
		end
		if data.Age or data['Max-Age'] then
			value = value .. '; Max-Age=' .. tostring(data.Age or data['Max-Age'])
		elseif data.Expires then
			if type(data.Expires) == 'string' then
				value = value .. '; Expires=' .. data.Expires
			else
				value = value .. '; Expires=' .. timestamp.format(data.Expires)
			end
		end
		cooks[name] = value
	end
	function res:finish_headers()
		if self._cookies then
			local cooks = {}
			for k,v in pairs(self._cookies) do
				table.insert(cooks,k .. '=' .. v)
			end
			table.sort(cooks)
			for _,v in ipairs(cooks) do
				self:add_header('Set-Cookie',v)
			end
		end
	end
end

function cookie:use(web)
	web:register_handler(function(req,res)
		self:handle_request(req,res)
	end)
end


return cookie
