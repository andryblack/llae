local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'
local template = require 'llae.template'
local utils = require 'llae.utils'

local views = class(nil,'static')

function views:_init( root, options )
	self._root = root
	self._options = options
	self._ext = (options and options.ext) or 'thtml' 
	self._cache = {}
end

function views:check( fn )
	local s = fs.stat(fn)
	return s and s.isfile
end

function views:get( view )
	local t = self._cache[view]
	if t and not self._options.nocache then
		return t
	end
	local fn = self._app:get_fs_path(path.join(self._root,view .. '.' .. self._ext))
	log.debug('load template',fn)
	t = template.load(fn,self._options)
	self._cache[view] = t
	return t
end

function views:use( app )
	self._app = app
	local sself = self
	app:register_handler( function (request, resp )
		resp.render = function (_,view, ...) 
			local t = sself:get(view)
			resp:set_header("Content-Type", "text/html")
			resp:finish(t(utils.merge(sself._options.env,...)))
		end
	end)
end

return views