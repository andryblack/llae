local class = require 'llae.class'
local path = require 'llae.path'
local fs = require 'llae.fs'
local log = require 'llae.log'
local git = class(require 'modules.base')


function git.load(project,url,install)

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
	if name:sub(-4) == '.git' then
		name = name:sub(1,-5)
	end
	local config = {}
	if options then
		for w in string.gmatch(options,'[^;]+') do
			local k,v = string.match(w, "(%w+)=(.+)")
			config[k]=v
		end
	end
	local proto = config.proto or 'git'
	local m = require 'modules.functions'
	local tag = config.tag or config.branch or 'master'
	local dst = tag
	local root = project:get_root()
	if not config.dir then
		config.dir = name .. '-' .. dst
	end
	local fm = {
		location = path.join(root,'build','modules',name),
	}
	fs.mkdir_r(fm.location)
	if install then
		if proto ~= 'git' then
			proto = proto .. '://'
		else
			proto = ''
		end
		m.download_git(fm,proto .. host .. '/' .. rpath,config)
	end
	local fn = path.join(fm.location,config.dir,'llae-module.lua')
	local dir = path.join(fm.location,config.dir,'modules')
	if fs.isdir(dir) then
		project:add_modules_location(dir)
	end
	if fs.isfile(fn) then

		local mod = git.new( name )
		mod:set_root(root)
		mod:loadfile(fn,project)
		mod:set_env('dir',config.dir)
		return mod
	else
		log.error('not found git module file',fn)
		return false, 'not found module file ' .. tostring(fn)
	end
end

return git