local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local fs = require 'llae.fs'
local log = require 'llae.log'
local uv = require 'uv'


local upgrade = class(tool)
upgrade.descr = 'upgrade llae installation'
local default_dir = path.join(assert(fs.home()),'.llae') 

function upgrade:exec( args )
	
	local install_dir = self.get_default_root()

	log.info('start upgrade at',install_dir)

	utils.run(function()
		if not fs.isdir(install_dir) then
			error('not found installation dir')
		end

		local src = assert(uv.exepath())
		local fn = path.basename(src)

		local modules = require 'modules'
		local project = require 'project'
		local p = project.new({},install_dir)
		p:add_module('llae')
		p:install_modules(true)
		for _,m in p:foreach_module() do
			if m.upgrade then
				m.upgrade{
					install_root = install_dir
				}
			end
		end

		log.info('done')
	end)

end

return upgrade