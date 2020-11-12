local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local fs = require 'llae.fs'
local log = require 'llae.log'
local uv = require 'uv'


local install = class(tool)
install.descr = 'bootstrap llae installation'
local default_dir = path.join(assert(fs.home()),'.llae') 

local embedded_modules = {
	'premake',
	'llae',
}

function install:exec( args )
	
	local install_dir = default_dir

	install_dir = utils.replace_env(install_dir)
	log.info('start bootstapping at',install_dir)

	utils.run(function()
		fs.mkdir(install_dir)

		fs.mkdir(path.join(install_dir,'bin'))
		fs.mkdir(path.join(install_dir,'modules'))
		fs.mkdir(path.join(install_dir,'scripts'))
		fs.mkdir(path.join(install_dir,'data'))

		local src = assert(uv.exepath())
		local fn = path.basename(src)

		assert(fs.copyfile(src,path.join(install_dir,'bin',fn)))

		local modules = require 'modules'
		for _,v in ipairs(embedded_modules) do
			local m = modules.get(install_dir,v)
			modules.install(m,install_dir)
			if m.bootstrap then
				m.bootstrap{
					install_root = install_dir
				}
			end
		end

		log.info('done')
	end)

end

return install