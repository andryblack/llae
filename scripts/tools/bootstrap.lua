local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local fs = require 'llae.fs'
local uv = require 'uv'


local install = class(tool)
install.descr = 'bootstrap llae installation'
local default_dir = path.join(fs.home(),'.llae') 

function install:exec( args )
	print('bootstrap:',args)

	local install_dir = default_dir

	install_dir = utils.replace_env(install_dir)
	print('start bootstapping at',install_dir)

	utils.run(function()
		fs.mkdir(install_dir)

		fs.mkdir(path.join(install_dir,'bin'))
		fs.mkdir(path.join(install_dir,'modules'))
		fs.mkdir(path.join(install_dir,'scripts'))

		local src = assert(uv.exepath())
		local fn = path.basename(src)

		assert(fs.copyfile(src,path.join(install_dir,'bin',fn)))

		print('done')
	end)

end

return install