local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local utils = require 'llae.utils'
local tool = class()

local vars = {
	bindir = path.dirname(fs.exepath())
}
local function replace_vars(str)
	return string.gsub(str,"{(.-)}",vars)
end

tool._llae_root = LLAE_ROOT and replace_vars(LLAE_ROOT) or path.join(vars.bindir,'..')

function tool.set_root( root )
	tool._llae_root = root
end

function tool:exec( args  )
	-- body
end

function tool.get_llae_path( ... )
	return path.join(tool._llae_root,...)
end

function tool.get_default_root()
	local default_dir = path.join(assert(fs.home()),'.llae') 
	local install_dir = default_dir
	install_dir = utils.replace_env(install_dir)
	return install_dir
end

return tool