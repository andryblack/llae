local class = require 'llae.class'
local json = require 'json'

---@class web.json : web.middle
---@field new fun(): web.json
local middle = class(require 'web.middle','web.json')

function middle:_init( )
end

function middle:use( app )
	self._app = app
	app:register_handler( function (request, resp )
		local t = request:get_header('Content-Type')
		if t == 'application/json' then
			request.body = request.body or request:read_body()
			request.json,request.json_error = json.decode(request.body,true)
		end
		resp.json = function (self,data)
			self:set_header("Content-Type", "application/json")
			return self:finish(json.encode(data))
		end
	end)
end

return middle