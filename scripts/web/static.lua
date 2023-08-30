local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'

local static = class(require 'web.middle','static')


function static:_init( root, options )
	self._root = root
	self._options = options or {}
	self._path = (self._options.path or '/')
	self._route = self._path  .. '*path'
	self._check_cache = {}
	if self._options.extensions then
		self._extensions = {}
		for _,v in ipairs(self._options.extensions) do
			self._extensions[v] = true
		end
	end
end

function static:get_fs_path(rpath)
	local apath = self._root and path.join(self._root,rpath) or rpath
	return self._app:get_fs_path(apath)
end

function static:check( fn )
	local v = self._check_cache[fn]
	if type(v) ~= 'nil' then
		return v
	end
	local s = fs.stat(fn)
	if not s or not s.isfile then
		self._check_cache[fn] = false
		return false
	end
	if self._extensions then
		if not self._extensions[path.extension(fn)] then
			self._check_cache[fn] = false
			return false
		end
	end
	self._check_cache[fn] = true
	return true
end

function static:is_static(rpath)
	if string.sub(rpath,1,#self._path) ~= self._path then
		return false
	end
	local fn = self._app:get_fs_path(path.join(self._root,rpath))
	if self:check(fn) then
		return true
	end
	return false
end

function static:use( app )
	self._app = app
	app:get(self._route,function(req,res,next)
		local fn = self._app:get_fs_path(path.join(self._root,req.params.path))
		if self:check(fn) then
			return res:send_static_file(fn)
		end
		log.debug('not found file',fn)
		return next()
	end)
end

return static