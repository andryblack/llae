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
	self._cache_parts = {}
	self._funcs = {}

	self._funcs.include = function (view,...) 
		local t = self:get(view)
		return t(utils.merge(self._options.env,self._funcs,...))
	end

	self._funcs.include_parts = function (view,...) 
		local t = self:get_parts(view)
		return t
	end
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

function views:get_parts( view )
	local t = self._cache_parts[view]
	if t and not self._options.nocache then
		return t
	end
	local fn = self._app:get_fs_path(path.join(self._root,view .. '.' .. self._ext))
	--log.debug('load parts template',fn)
	local data = tostring(fs.load_file(fn))

	t = {}
	local pos = 1
	local name = 'base'
	while true do
		local npos,epos = string.find(data,'-{.+}%-',pos)
		local part = string.sub(data,pos,npos and npos-1)
		local r = template.compile(part,self._options)
		--log.info('compile part',name)
		t[name] = function(...)
			return r(utils.merge(self._options.env,self._funcs,...))
		end
		if not npos then
			break
		end
		name = string.match(string.sub(data,npos,epos),'-{(.+)}%-')
		pos = epos + 1
	end

	self._cache_parts[view] = t
	return t
end


function views:use( app )
	self._app = app
	local sself = self
	app:register_handler( function (request, resp )
		resp.render = function (_,view, ...) 
			local t = sself:get(view)
			resp:set_header("Content-Type", "text/html")
			resp:finish(t(utils.merge(sself._options.env,sself._funcs,...)))
		end
	end)
end

return views