local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'

local static = class(nil,'static')


function static:_init( root, options )
	self._root = root
	self._options = options
end

function static:check( fn )
	local s = fs.stat(fn)
	return s and s.isfile
end

function static:use( app )
	self._app = app
	app:get('*path',function(req,res,next)
		local fn = path.join(self._root,req.params.path)
		if self:check(fn) then
			return res:send_static_file(fn)
		end
		log.debug('not found file',fn)
		return next()
	end)
end

return static