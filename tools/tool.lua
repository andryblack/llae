local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local tool = class()

tool._llae_root = path.join(path.dirname(fs.exepath()),'..')

function tool.set_root( root )
	tool._llae_root = root
end

function tool:exec( args  )
	-- body
end

function tool.get_llae_path( ... )
	return path.join(tool._llae_root,...)
end

return tool