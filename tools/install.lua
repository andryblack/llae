local tool = require 'tool'
local class = require  'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'

local install = class(tool)
install.descr = 'install module'
install.args = {
	{'file','install module from file', true },
	{'modules','additional modules location', true },
	{'','module name',false}
}


function install:exec( args )
	print('install:',args)
	local utils = require 'llae.utils'
	if args.file then
		log.info('install file:',args.file)
		local m = require 'modules'
		utils.run(function()
			m.install_file(args.file,utils.replace_env(fs.pwd()))
		end)
		return
	end
	local modname = args[2]
	local Project = require 'project'
	local prj,err = Project.load('llae-project.lua')
	if not prj then
		error('failed loading project file ' .. err)
	end
	if args.modules then
		prj:add_modules_location(args.modules)
	end
	utils.run(function()
		if modname and modname ~= '' then
			log.info('install module')
			prj:install_module(modname)
		else
	 		prj:install_modules()
			prj:write_premake()
		end
	end)
end

return install