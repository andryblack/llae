local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local fs = require 'llae.fs'
local uv = require 'uv'


local install = class(tool)
install.descr = 'bootstrap llae installation'
local default_dir = path.join(assert(fs.home()),'.llae') 

local embedded_modules = {
	'llae',
	'premake'
}

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

		-- for _,v in ipairs(embedded_modules) do
		-- 	local cont = assert(package.get_embedded('modules.'..v),'not found embedded module ' .. v)
		-- 	fs.write_file(path.join(install_dir,'modules',v..'.lua'),cont)
		-- end

		print('done')
	end)

end

return install