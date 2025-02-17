local class = require 'llae.class'
local modules = require 'modules'
local fs = require 'llae.fs'
local template = require 'llae.template'
local path = require 'llae.path'
local log = require 'llae.log'
local utils = require 'llae.utils'
local llae = require 'llae'
local tool = require 'tool'


local Project = class(nil,'Project')


function Project.get_path(base,part)
	--print('get_path',base,part)
	if path.isabsolute(part) then
		return part
	end
	return path.join(base,part)
end

Project.env = {}

function Project.env:project( name )
	if type(name) ~= 'string' then
		error('project must be string')
	end
	self.project_name = name
end

function Project.env:module( name )
	if type(name) ~= 'string' then
		error('module must be string')
	end
	table.insert(self.modules,name)
end

function Project.env:premake( data )
	if type(data) ~= 'table' then
		error('premake must be table')
	end
	self.premake = data
end

function Project.env:cmodule( data )
	if not self.cmodules then
		self.cmodules = {}
	end 
	table.insert(self.cmodules,data)
end

function Project.env:config( module, name, value )
	if not self.module_config then
		self.module_config = {}
	end
	table.insert(self.module_config,{
		module = assert(module),
		name = assert(name),
		value = value
	})
end

function Project.env:generate_src( data )
	if type(data) ~= 'table' then
		error('generate_src must be table')
	end
	if not self.generate_src then
		self.generate_src = { data }
	else 
		table.insert(self.generate_src,data)
	end
end

function Project.env:command(data)
	if type(data) ~= 'table' then
		error('command must be table')
	end
	if not data.name then
		error('command need name')
	end
	if not data.script then
		error('command need script')
	end
	if not self.commands then
		self.commands = { data }
	else
		table.insert(self.commands,data)
	end
end


function Project.env:print(...)
	log.info('[project]',...)
end

local function get_target(cmdargs)
	return  (cmdargs and cmdargs['target-platform']) or os.getenv('LLAE_TARGET_PLATFORM') or llae.get_host_platform()
end

function Project:_init( env  )
	log.debug('Project:_init')
	self._env = env
	self._env.project = self
	self._scripts = {}
	self._modules_locations = {}
	self._modules = {}
	self._modules_list = {}
	self._cmodules = {}
	self._module_config = env.module_config or {}
	
	if self._env.location then
		self:add_modules_location(path.join(self._env.location,'modules'))
	end
	if self._env.cmodules then
		for _,v in ipairs(self._env.cmodules) do
			self:add_cmodule(v)
		end
	end
	local cmdargs = self._env.cmdargs
	self._dl_dir = (cmdargs and cmdargs['dl-dir']) or os.getenv('LLAE_DL_DIR') or tool.get_llae_path('dl')
	self._target = get_target( cmdargs )
end

function Project:get_dl_dir()
	return self._dl_dir
end

function Project:name(  )
	return self._env.project_name
end

function Project:get_premake( )
	return self._env.premake
end

function Project:get_root( )
	return self._env.location or fs.pwd()
end

function Project:get_cmdargs(  )
	return self._env.cmdargs or {}
end

function Project:get_commands( )
	return self._env.commands
end

function Project:get_target_platform()
	return self._target
end

function Project:get_host_platform()
	return llae.get_host_platform()
end

function Project:add_modules_location( loc )
	log.debug('add modules location',loc)
	table.insert(self._modules_locations,loc)
end

function Project:get_modules_locations() 
	return self._modules_locations
end

function Project:add_cmodule(cmod)
	if type(cmod) == 'string' then
		table.insert(self._cmodules,{name=cmod,func='luaopen_' .. string.gsub(cmod,'[%.%-]','_')})
	else
		table.insert(self._cmodules,{name=cmod[1],func=cmod[2]})
	end
end

function Project:add_module( name , install)
	if self._modules[name] then
		return
	end
	local m = modules.get(self,name, install)
	m:set_root(self:get_root())
	m:set_project(self)
		
	self._modules[name] = m
	
	if m:get_dependencies() then
		for _,v in ipairs(m:get_dependencies()) do
			self:add_module(v, install)
		end
	end
	table.insert(self._modules_list,m)
	local cmodules = m:get_cmodules()
	if cmodules then
		for _,cmod in ipairs(cmodules) do
			self:add_cmodule(cmod)
		end
	end
	m:load_configs(self._module_config)
end

function Project:init_modules()
	for _,m in ipairs(self._modules_list) do
		m:resolve_configs(self._module_config)
	end
end


function Project:load_modules( install )
	if next(self._modules) then
		return
	end
	self._modules = {}
	self._modules_list = {}
	for _,n in ipairs(self._env.modules) do
		self:add_module(n,install)
	end
	self:init_modules()
end
function Project:install_modules( tosystem )
	self:load_modules(true)
	self._scripts = {}
	local root = self:get_root()
	fs.mkdir(path.join(root,'build'))
	fs.mkdir(path.join(root,'build','modules'))
	fs.mkdir(path.join(root,'build','premake'))
	fs.mkdir(self:get_dl_dir())
	for _,m in ipairs(self._modules_list) do
		m:install(tosystem)
	end
	
end

local function apply_functions( super_env, env )
	local m = require 'modules.functions'
	for n,v in pairs(m) do
		super_env[n] = function(...)
			return v(env,...)
		end
	end
end

function Project:install(tosystem)
	self:install_modules(tosystem)
	if self._env.__write_env and self._env.__write_env.install then
		local funcs = {
			print = function(...)
				Project.env.print(self._env,...)
			end
		}
		apply_functions(funcs,self._env)
		setmetatable(self._env.__load_env,{
			__index = setmetatable(funcs,{__index=self._env}),
			__newindex={}
		})
		--modules.apply_functions(self._env.__write_env,self._env)
		self._env.__write_env.install()
	end
end

function Project:install_module( name )
	self:load_modules(true)
	self._scripts = {}
	local root = self:get_root()
	fs.mkdir(path.join(root,'build'))
	fs.mkdir(path.join(root,'build','modules'))
	fs.mkdir(path.join(root,'build','premake'))
	local m = self._modules[name]
	m:install(root)
end

function Project:check_script( file , m )
	if self._scripts[file] then
		log.error('reqrite script',file,'from module',m.name)
		log.error('already installed by module',self._scripts[file].name)
		error('script rewrite: ' .. file)
	end
	self._scripts[file] = m
end

local function mod_next(t,i)
	i = i + 1
	if t[i] then
		return i, t[i]:get_env()
	end
end

function Project:foreach_module( )
	return mod_next,self._modules_list,0
end

local function reversed_mod_next(t, i)
    i = i - 1
    if i ~= 0 then
        return i, t[i]:get_env()
    end
end

function Project:foreach_module_rev( )
	return reversed_mod_next,self._modules_list,#self._modules_list + 1
end

function Project:get_module( name )
	local res = self._modules[name]
	if res then
		return res
	end
	for _,v in pairs(self._modules) do
		if v:get_name() == name then
			return v
		end
	end
end

function Project:get_cmodules(  )
	return self._cmodules
end

function Project:get_config_value( module_name, config_name )
	local module = self:get_module(module_name)
	if not module then
		error('module not connected: ' .. tostring(module_name))
		return nil
	end
	local config = module:get_config(config_name)
	return config.value
end

function Project:write_premake(  )
	local template_source_filename = tool.get_llae_path('data','premake5-template.lua')
	local filename = path.join(self:get_root(),'build','premake5.lua')
	log.info('generate premake5.lua')
	fs.mkdir_r(path.dirname(filename))
	fs.unlink(filename)
	local f = assert(fs.open(filename,fs.O_WRONLY|fs.O_CREAT))
	f:write(template.render_file(template_source_filename,{
		escape = tostring,
		project=self,
		template = template,
		path = path,
		fs = fs,
		log = log,
		utils = utils
	}))
	f:close()
end

function Project:write_generated( )
	self:load_modules()
	self:write_premake()
	
	for _,conf in ipairs(self._env.generate_src or {}) do
		local template_f
		if conf.template then
			local template_source_filename = path.join(self:get_root(),conf.template)
			template_f = template.load(template_source_filename)
		else
			template_f = template.compile(conf.template_content)
		end
		local filename = path.join(self:get_root(),conf.filename)
		log.info('generate',conf.filename)
		fs.mkdir_r(path.dirname(filename))
		fs.unlink(filename)
		local f = assert(fs.open(filename,fs.O_WRONLY|fs.O_CREAT))
		local ctx = setmetatable({
			escape = conf.escape or tostring,
			project=self,
			template = template,
			path = path,
			conf = conf,
			fs = fs,
			root = self:get_root(),
			log = log
		},{__index=_G})
		if conf.config then
			load(conf.config,'generate:config','t',ctx)()
		end
		f:write( template_f(ctx) )
		f:close()
	end

	for _,m in ipairs(self._modules_list) do
		for _,conf in ipairs(m:get_generate_src()) do
			local template_f
			if conf.template then
				local template_source_filename = Project.get_path(m:get_location(),conf.template)
				template_f = template.load(template_source_filename)
			else
				template_f = template.compile(conf.template_content)
			end
			local filename = path.join(self:get_root(),conf.filename)
			log.info('generate',conf.filename)
			fs.mkdir_r(path.dirname(filename))
			fs.unlink(filename)
			local f = assert(fs.open(filename,fs.O_WRONLY|fs.O_CREAT))
			local ctx = setmetatable( {
				escape = conf.escape or tostring,
				project=self,
				template = template,
				path = path,
				conf = conf,
				fs = fs,
				log = log
			},{__index=m:get_env()})
			if conf.config then
				load(conf.config,'generate:config','t',ctx)()
			end
			f:write( template_f(ctx) )
			f:close()
		end
	end
	

end

local function create_env( cmdargs  )
	local env = {
		modules = {},
	}
	local global_env = {}
	local write_env = {}
	for n,v in pairs(Project.env) do
		global_env[n] = function(...)
			v(env,...)
		end
	end
	setmetatable(global_env,{__index=env})
	return setmetatable( {},  {__index=global_env,__newindex=write_env} ), env, write_env
end

function Project.load( root_dir , cmdargs )
	if root_dir then
		root_dir = path.join(fs.pwd(),root_dir)
	else
		root_dir = fs.pwd()
	end
	local load_env,env,write_env = create_env( cmdargs )
	env.location = root_dir
	env.root = env.location
	env.cmdargs = cmdargs
	env.target = get_target( cmdargs )
	env.host = llae.get_host_platform()
	env.__write_env = write_env
	env.__load_env = load_env
	local res,err = loadfile(path.join(root_dir,'llae-project.lua'),'bt',load_env)
	if not res then
		return res,err
	end
	res,err = pcall(res)
	if not res then
		return res,err
	end

	-- validate
	if not env.project_name then
		return nil,'need project name'
	end

	log.debug('loaded project at',root_dir)
	return Project.new(env)
end

return Project