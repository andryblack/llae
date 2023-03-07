local tool = require 'tool'
local class = require  'llae.class'
local async = require 'llae.async'
local path = require 'llae.path'
local fs = require 'llae.fs'
local log = require 'llae.log'


local install = class(tool)
install.descr = 'bootstrap llae installation'

local embedded_modules = {
	'premake',
	'llae',
}

function install:exec( args )
	
	local install_dir = self.get_default_root()
	log.info('start bootstapping at',install_dir)

	async.run(function()
		fs.mkdir(install_dir)

		fs.mkdir(path.join(install_dir,'bin'))
		fs.mkdir(path.join(install_dir,'modules'))
		fs.mkdir(path.join(install_dir,'scripts'))
		fs.mkdir(path.join(install_dir,'data'))

		local src = assert(fs.exepath())
		local fn = path.basename(src)

		--assert(fs.copyfile(src,path.join(install_dir,'bin',fn)))

		local modules = require 'modules'
		local project = require 'project'
		local p = project.new({},install_dir)
		for _,v in ipairs(embedded_modules) do
			p:add_module(v)
		end
		p:install_modules(true)
		for _,m in p:foreach_module() do
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