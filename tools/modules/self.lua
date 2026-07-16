local class = require 'llae.class'
local path = require 'llae.path'
local fs = require 'llae.fs'
local log = require 'llae.log'

---@class modules.self_module : modules.base
---@field new fun(name: string): modules.self_module
local self_module = class(require 'modules.base')

---@param project Project
---@param name string
---@return modules.self_module?
---@return string?
function self_module.load(project,name)

	local root = project:get_root()
	local fn = path.join(root,'llae-module.lua')
	if fs.isfile(fn) then
		local mod = self_module.new( name )
		mod:set_root(root)
		mod:create_env(project)
		mod:set_env('is_self',true)
		mod:set_env('dir','.')
		mod:loadfile(fn,project)
		return mod
	else
		return nil, 'not found module file ' .. tostring(fn)
	end
end

function self_module:on_root_set()
	self._location = path.join(self._root)
end

function self_module:install()
	log.info('install self module',self:get_name(),self._env.version)
	if self._env.self_install then
		self._env.self_install()
	end
end

return self_module