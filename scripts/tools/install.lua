local tool = require 'tool'
local class = require  'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'

local install = class(tool)
install.descr = 'install module'
install.args = {
	{'file','install module from file', true },
}


function install:exec( args )
	print('install:',args)
	local utils = require 'llae.utils'
	if args.file then
		print('install file:',args.file)
		local m = require 'module'
		utils.run(function()
			m.install_file(args.file,utils.replace_env(fs.pwd()))
		end)
		
	end
end

return install