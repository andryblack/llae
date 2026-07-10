local tool = require 'tool'
local class = require  'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'

local install = class(tool)
install.descr = 'local modules versions'
install.args = {
	{'project-dir','project dir',true},
    {'modules','additional modules location', true },
}


function install:exec( args )
	log.info('lock:',args)
	local async = require 'llae.async'
	local Project = require 'project'
	local prj,err = Project.load( args['project-dir'] , args)
	if not prj then
		error('failed loading project file ' .. err)
	end
	if args.modules then
		prj:add_modules_location(args.modules)
	end
	async.run(function()
		prj:update_modules()
	end)
end

return install