local tool = require 'tool'
local class = require  'llae.class'

local install = class(tool)
install.descr = 'install module'
install.args = {
	{'f','install module from file', true },
}

function install:exec( args )
	print('install:',args)
end

return install