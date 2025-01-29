local path = require 'llae.path'
local fs = require 'llae.fs'
local utils = require 'llae.utils'
local async = require 'llae.async'
local os = require 'llae.os'
local http = require 'net.http'
local log = require 'llae.log'
local untar = require 'archive.tar'
local unzip = require 'archive.zip'
local tool = require 'tool'
local crypto = require 'llae.crypto'
local uv = require 'uv'

local modules_base = require 'modules.base'
local modules_git = require 'modules.git'
local modules_file = require 'modules.file'


local _M = {  }

function _M.loadfile( filename , project )
	local m = modules_base.new( 'file:'..filename )
	m:loadfile(filename,project)
	return m
end


function _M.install_file( filename, root )
	local m = _M.loadfile( filename )
	m:set_root(root)
	m:install()
	local dst = path.getabsolute(path.join(root,'modules', m:get_name().. '.lua'))
	local src = path.getabsolute(filename)
	if src ~= dst then
		log.info('install',src,'->',dst)
		fs.mkdir_r(path.dirname(dst))
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
	return m
end


function _M.get( project, modname, install )

	local proto,tail = string.match(modname,'^(%l+)://(.*)$')
	if proto and proto=='git' then
		local mod,err = modules_git.load(project,tail,install)
		if mod then
			return mod
		else
			log.error('failed load git module: ',modname)
			log.error(err)
			error('failed load git module ' .. modname)
		end
	elseif proto and proto == 'file' then
		local mod,err = modules_file.load(project,tail,install)
		if mod then
			return mod
		else
			log.error('failed load file module: ',modname)
			log.error(err)
			error('failed load file module ' .. modname)
		end
	end

	local mod
	local root
	local locations = project:get_modules_locations()
	for _,v in ipairs(locations) do
		local fn = path.join(v,modname .. '.lua')
		if fs.isfile(fn) then
			mod = _M.loadfile( fn , project )
			mod.source = fn
			break
		else
			log.debug('not found module at',fn)
		end
	end
	if not mod then
		local fn = tool.get_llae_path('modules',modname .. '.lua') 
		if fs.isfile(fn) then
			mod =  _M.loadfile(fn , project)
			mod.source = fn
		end
	end

	if mod then
		return mod
	end
	log.error('not found module ' , modname)
	for _,v in ipairs(locations) do
		log.error('\t','at',v)
	end
	log.error('\t','at',tool.get_llae_path('modules'))
	error('not found module ' .. modname)
end

return _M