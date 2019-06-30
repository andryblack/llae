local tool = require 'tool'
local class = require  'llae.class'
local utils = require 'llae.utils'
local path = require 'llae.path'
local llae = require 'llae'

local install = class(tool)
install.descr = 'bootstrap llae installation'
local default_dir = path.join(llae.get_home(),'.llae') 

function install:exec( args )
	print('bootstrap:',args)

	local install_dir = default_dir

	install_dir = utils.replace_env(install_dir)
	print('start bootstapping at',install_dir)

	utils.run(function()
		path.mkdir(install_dir)

		path.mkdir(path.join(install_dir,'bin'))
		path.mkdir(path.join(install_dir,'bin'))

	end)

end

return install