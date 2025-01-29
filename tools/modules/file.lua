local class = require 'llae.class'
local path = require 'llae.path'
local fs = require 'llae.fs'

local file = class(require 'modules.base')

function file:_init(name, location)
	file.baseclass._init(self,name)
	self._location = location
end

function file.load(project,url,install)

	local host,rpath,options = string.match(url,'^(.+)/([^;]+);(.*)$')
	if not host then
		host,rpath = string.match(url,'^(.+)/(.+)$')
	end
	if not host then
		return false, 'failed parse url'
	end
	local name = path.basename(rpath)
	if not name then
		return false, 'failed find name'
	end

	local config = {}
	if options then
		for w in string.gmatch(options,'[^;]+') do
			local k,v = string.match(w, "(%w+)=(%w+)")
			config[k]=v
		end
	end
	local m = require 'modules.functions'
	local dst = tag
	local root = project:get_root()
	local fm = {
		location = path.join(root,host,rpath),
	}
	assert(fs.isdir(fm.location))
	if install then
		
	end
	local fn = path.join(fm.location,'llae-module.lua')
	local dir = path.join(fm.location,'modules')
	if fs.isdir(dir) then
		project:add_modules_location(dir)
	end
	if fs.isfile(fn) then

		local mod = file.new( name , fm.location )
		mod:set_root(root)
		mod:loadfile(fn,project)
		mod:set_env('dir','')
		return mod
	else
		return false, 'not found module file ' .. tostring(fn)
	end
end

function file:on_root_set()
	--self._location = self._env.location
end

return file