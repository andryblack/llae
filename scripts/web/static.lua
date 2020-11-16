local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'

local static = class(nil,'static')


function static:_init( root, options )
	self._root = root
	self._options = options or {}
	self._route = (self._options.path or '/') .. '*path'
	if self._options.extensions then
		self._extensions = {}
		for _,v in ipairs(self._options.extensions) do
			self._extensions[v] = true
		end
	end
end

function static:check( fn )
	local s = fs.stat(fn)
	if not s or not s.isfile then
		return false
	end
	if self._extensions then
		return self._extensions[path.extension(fn)]
	end
	return true
end

function static:use( app )
	self._app = app
	app:get(self._route,function(req,res,next)
		local fn = path.join(self._root,req.params.path)
		if self:check(fn) then
			return res:send_static_file(fn)
		end
		log.debug('not found file',fn)
		return next()
	end)
end

return static