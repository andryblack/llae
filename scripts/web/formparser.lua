local class = require 'llae.class'
local url = require 'net.url'
local media_type = require 'web.media_type'

---@class web.formparser : web.middle
---@field new fun():web.formparser
local formparser = class(require 'web.middle','formparser')

function formparser.parse(str)
	return url.parseQuery(str)
end


function formparser.build(values)
	return url.buildQuery(values)
end

function formparser:handle_request(req,res)
	local ct = req:get_header('Content-Type')
	if media_type.is(ct, 'application/x-www-form-urlencoded') then
		req.form = formparser.parse(req:read_body())
	end
end

function formparser:use(web)
	web:register_handler(function(req,res)
		self:handle_request(req,res)
	end)
end


return formparser
