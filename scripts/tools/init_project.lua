local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local fs = require 'llae.fs'
local uv = require 'uv'


local cmd = class(tool)
cmd.descr = 'init project'
cmd.args = {
	{'','project name', true },
}
function cmd:exec( args )
	print('init:',args)

	local install_dir = path.join(fs.pwd(),args[2]) 

	install_dir = utils.replace_env(install_dir)
	print('start init project at',install_dir)

	utils.run(function()
		fs.mkdir(install_dir)

		fs.mkdir(path.join(install_dir,'bin'))
		fs.mkdir(path.join(install_dir,'modules'))
		fs.mkdir(path.join(install_dir,'scripts'))

		local src = assert(uv.exepath())
		local fn = path.basename(src)

		assert(fs.copyfile(src,path.join(install_dir,'bin',fn)))

		local m = require 'module'
		m.install_file('modules/premake.lua',install_dir)
		m.install_file('modules/llae.lua',install_dir)

		local proj_f = path.join(install_dir,'llae-project.lua')

		print('done')
	end)

end

return cmd