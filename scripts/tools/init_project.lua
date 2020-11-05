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

	local proj_name = args[2]
	local install_dir = path.join(fs.pwd(),proj_name) 

	install_dir = utils.replace_env(install_dir)
	print('start init project at',install_dir)

	utils.run(function()
		fs.mkdir(install_dir)

		fs.mkdir(path.join(install_dir,'bin'))
		fs.mkdir(path.join(install_dir,'modules'))
		fs.mkdir(path.join(install_dir,'scripts'))
		fs.mkdir(path.join(install_dir,'build'))

		local src = assert(uv.exepath())
		local fn = path.basename(src)

		assert(fs.copyfile(src,path.join(install_dir,'bin',fn)))

		fs.write_file(path.join(install_dir,'.gitignore'),[[
/bin/
/build/
]])

fs.write_file(path.join(install_dir,'llae-project.lua'),utils.replace_tokens([[
-- project ${proj_name}
project '${proj_name}'
-- @modules@
module 'llae'
]],{
		proj_name = proj_name,
}))
		
		print('done')
	end)

end

return cmd