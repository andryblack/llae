local class = require 'llae.class'
local modules = require 'modules'
local fs = require 'llae.fs'

local Project = class(nil,'Project')

Project.env = {}

function Project.env:project( name )
	if type(name) ~= 'string' then
		error('project must be string')
	end
	self.project = name
end

function Project.env:module( name )
	if type(name) ~= 'string' then
		error('module must be string')
	end
	table.insert(self.modules,name)
end

function Project:_init( env , root)
	self._env = env
	self._root = root
end

function Project:add_module( name )
	if self._modules[name] then
		return
	end
	local m = modules.get(self._root,name)
	self._modules[name] = m
	if m.dependencies then
		for _,v in ipairs(m.dependencies) do
			self:add_module(v)
		end
	end
	table.insert(self._modules_list,m)
end

function Project:load_modules(  )
	if self._modules then
		return
	end
	self._modules = {}
	self._modules_list = {}
	for _,n in ipairs(self._env.modules) do
		self:add_module(n)
	end
end
function Project:install_modules(  )
	self:load_modules()
	for _,m in ipairs(self._modules_list) do
		modules.install(m)
	end
end

local function create_env( )
	local env = {
		modules = {}
	}
	local global_env = {}
	local super_env = {}
	for n,v in pairs(Project.env) do
		super_env[n] = function(...)
			v(env,...)
		end
	end
	return setmetatable( global_env, {__index=super_env} ), env
end

function Project.load( )
	local load_env,env = create_env( )
	local res,err = loadfile('llae-project.lua','bt',load_env)
	if not res then
		return res,err
	end
	res,err = pcall(res)
	if not res then
		return res,err
	end

	-- validate
	if not env.project then
		return nil,'need project'
	end

	return Project.new(env,fs.pwd())
end

return Project