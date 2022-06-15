local class = require 'llae.class'
local url = require 'net.url'

local formparser = class(require 'web.middle','formparser')

function formparser.parse(str)
	return url.parseQuery(str)
end


function formparser.build(values)
	return url.buildQuery(values)
end

function formparser:handle_request(req,res)
	local ct = req:get_header('Content-Type')
	if ct and ct == 'application/x-www-form-urlencoded' then
		req.form = formparser.parse(req:read_body())
	end
end

function formparser:use(web)
	web:register_handler(function(req,res)
		self:handle_request(req,res)
	end)
end


return formparser
