local class = require 'llae.class'
local path = require 'llae.path'
local fs = require 'llae.fs'
local utils = require 'llae.utils'
local async = require 'llae.async'
local os = require 'llae.os'
local llae = require 'llae'

local log = require 'llae.log'

local tool = require 'tool'


local base = class()

function base:_init(name)
	self._name = name
end

function base:get_env()
	return self._env
end

function base:get_name()
	return (self._env and self._env.name or self._name) or 'module'
end

local function apply_functions( super_env, env )
	local m = require 'modules.functions'
	for n,v in pairs(m) do
		super_env[n] = function(...)
			return v(env,...)
		end
	end
end

function base:create_env(project)
	local env = {
		project = project,
		log = log,
		buffer = llae.buffer
	}
	local super_env = {}
	apply_functions(super_env,env)
	setmetatable( env, {__index=setmetatable(super_env,{__index=_G})} )
	self._env = env
end

function base:check_module( name )
	local res,err = pcall(function()
		if not self._env.name then
			error('need name')
		end
		if type(self._env.install) ~= 'function' then
			error('need install function')
		end
	end)
	if not res then
		error(err .. ' at ' .. name)
	end
	self._name = self._env.name
end

function base:update_env_location()
	if self._env then
		self._env.location = self._location
		self._env.root = self._root
	end
end

function base:on_root_set()
	self._location = path.join(self._root,'build','modules', self:get_name())
end

function base:set_root(root)
	self._root = path.getabsolute(root or self._root or utils.replace_env(fs.pwd()))
	self:on_root_set()
	self:update_env_location()	
end

function base:set_project(project)
	self._project = project
	if self._env then
		self._env._project = project
	end
end

function base:set_env(name,val)
	self._env[name] = val
end

function base:install(tosystem)
	log.info('install module',self:get_name(),self._env.version)
	os.setenv('LLAE_PROJECT_ROOT',self._root)
	self._env.tosystem = tosystem
	--fs.rmdir_r(env.location)
	fs.mkdir_r(self._env.location)
	self._env.install(tosystem)
end

function base:get_location()
	return self._location
end

function base:load_configs( module_configs )
	if self._env and self._env.module_configs then
		for _,c in ipairs(self._env.module_configs ) do
			table.insert(module_configs,c)
		end
	end
end

function base:resolve_configs( module_configs )
	local configs = {}
	self._configs = configs
	local lname = self:get_name()
	for _,v in ipairs(self._env.project_config or {}) do
		local name = assert(v[1],'need config name')
		if configs[name] then
			error('dublicate module config ' .. name)
		end
		configs[name] = v
		if v.storage and v.storage == 'list' then
			v.value = {}
			for __,cv in ipairs( module_configs ) do
				if cv.module == lname and cv.name == name then
					table.insert(v.value,cv.value)
				end
			end
		else
			for __,cv in ipairs( module_configs ) do
				if cv.module == lname and cv.name == name then
					if v.type and type(cv.value) ~= v.type then
						error('invalid config value type')
					end
					v.value = cv.value
				end
			end
		end
	end
end

function base:get_config(config_name)
	local config = self._configs[config_name]
	if not config then
		error('module ' .. self:get_name() .. ' dnt declare config: ' .. tostring(config_name))
	end
	return config
end

function base:get_generate_src()
	return self._env.generate_src or {}
end

function base:get_cmodules()
	return self._env.cmodules
end

function base:get_dependencies()
	return self._env.dependencies 
end

function base:loadfile(filename,project)
	self:create_env( project )
	assert(loadfile(filename,'bt',self._env))()
	self:check_module('file:' .. filename)
end

return base