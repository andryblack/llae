local tool = require 'tool'
local class = require  'llae.class'

local install = class(tool)
install.descr = 'install llae'

function install:exec( args )
	print('install:',args)
end

return install