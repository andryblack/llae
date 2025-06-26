local tool = require 'tool'
local class = require  'llae.class'

local version = class(tool)
version.descr = 'show LLAE version'
version.args = {}

function version:exec( args )
    print(LLAE_VERSION)
end

return version