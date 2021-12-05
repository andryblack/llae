local class = require 'llae.class'
local modules = require 'modules'
local fs = require 'llae.fs'
local template = require 'llae.template'
local path = require 'llae.path'
local log = require 'llae.log'
local utils = require 'llae.utils'
local tool = require 'tool'

local Project = class(nil,'Project')

Project.__llae_root = path.join(fs.exepath(),'..')
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
	if type(data) == 'string' then
		table.insert(self.cmodules,{name=data,func='luaopen_' .. string.gsub(data,'%.','_')})
	else
		table.insert(self.cmodules,{name=data[1],func=data[2]})
	end
end

function Project.env:config( module, config, value )
	if not self.module_config then
		self.module_config = {}
	end 
	local mod = self.module_config[module]
	if not mod then
		mod = {}
		self.module_config[module] = mod
	end
	local conf = mod[config]
	if not conf then
		conf = {}
		mod[config] = conf
	end
	table.insert(conf,value)
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

function Project:_init( env , root, cmdargs )
	log.debug('Project:_init',root,cmdargs)
	self._env = env
	self._root = root
	self._scripts = {}
	self._modules_locations = {}
	self._modules = {}
	self._modules_list = {}
	self._cmodules = {}
	self._cmdargs = cmdargs or {}
	if root then
		self:add_modules_location(path.join(root,'modules'))
	end
	self._dl_dir = (cmdargs and cmdargs['dl-dir']) or os.getenv('LLAE_DL_DIR') or tool.get_llae_path('dl')
end

function Project:get_dl_dir()
	return self._dl_dir
end

function Project:name(  )
	return self._env.project
end

function Project:get_premake( )
	return self._env.premake
end

function Project:get_root( )
	return self._root
end

function Project:get_cmdargs(  )
	return self._cmdargs
end

function Project:add_modules_location( loc )
	log.debug('add modules location',loc)
	table.insert(self._modules_locations,loc)
end

function Project:get_modules_locations() 
	return self._modules_locations
end

function Project:add_module( name )
	if self._modules[name] then
		return
	end
	local m = modules.get(self,name)
	
	m.root = self._root
	if self._root then
		m.location = path.join(self._root,'build','modules', m.name)
	end
		
	self._modules[name] = m
	m._project = self
	if m.dependencies then
		for _,v in ipairs(m.dependencies) do
			self:add_module(v)
		end
	end
	table.insert(self._modules_list,m)
	if m.cmodules then
		for _,cmod in ipairs(m.cmodules) do
			if type(cmod) == 'string' then
				table.insert(self._cmodules,{name=cmod,func='luaopen_' .. string.gsub(cmod,'%.','_')})
			else
				table.insert(self._cmodules,{name=cmod[1],func=cmod[2]})
			end
		end
	end
end

function Project:load_modules(  )
	if next(self._modules) then
		return
	end
	self._modules = {}
	self._modules_list = {}
	for _,n in ipairs(self._env.modules) do
		self:add_module(n)
	end
end
function Project:install_modules( tosystem )
	self:load_modules()
	self._scripts = {}
	local root = self._root or fs.pwd()
	fs.mkdir(path.join(root,'build'))
	fs.mkdir(path.join(root,'build','modules'))
	fs.mkdir(path.join(root,'build','premake'))
	fs.mkdir(self:get_dl_dir())
	for _,m in ipairs(self._modules_list) do
		modules.install(m,root,tosystem)
	end
end

function Project:install_module( name )
	self:load_modules()
	self._scripts = {}
	local root = self._root or fs.pwd()
	fs.mkdir(path.join(root,'build'))
	fs.mkdir(path.join(root,'build','modules'))
	fs.mkdir(path.join(root,'build','premake'))
	local m = self._modules[name]
	modules.install(m,root)
end

function Project:check_script( file , m )
	if self._scripts[file] then
		log.error('reqrite script',file,'from module',m.name)
		log.error('already installed by module',self._scripts[file].name)
		error('script rewrite: ' .. file)
	end
	self._scripts[file] = m
end

function Project:foreach_module( )
	return ipairs(self._modules_list)
end

function Project:foreach_module_rev( )
	return utils.reversedipairs(self._modules_list)
end

function Project:get_module( name )
	return self._modules[name]
end

function Project:get_cmodules(  )
	return utils.list_concat(self._cmodules,self._env.cmodules or {})
end

function Project:get_config( module, config )
	if not self._env.module_config then
		log.debug('get_config','empty module_config')
		return nil
	end
	local mc = self._env.module_config[module]
	if not mc then
		log.debug('get_config','empty module_config for ',module)
		return nil
	end
	return mc[config]
end

function Project:get_config_value( module, config )
	local l = self:get_config(module,config)
	return l and l[1]
end

function Project:write_premake(  )
	local template_source_filename = tool.get_llae_path('data','premake5-template.lua')
	local filename = path.join(self._root,'build','premake5.lua')
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
		log = log
	}))
	f:close()
end

function Project:write_generated( )
	self:load_modules()
	self:write_premake()
	for _,m in ipairs(self._modules_list) do
		for _,conf in ipairs(m.generate_src or {}) do
			m.root = self._root 
			m.location = path.join(m.root,'build','modules', m.name)
			local template_f
			if conf.template then
				local template_source_filename = path.join(m.location,conf.template)
				template_f = template.load(template_source_filename)
			else
				template_f = template.compile(conf.template_content)
			end
			local filename = path.join(m.root,conf.filename)
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
			},{__index=m})
			if conf.config then
				load(conf.config,'generate:config','t',ctx)()
			end
			f:write( template_f(ctx) )
			f:close()
		end
	end
	for _,conf in ipairs(self._env.generate_src or {}) do
		local template_f
		if conf.template then
			local template_source_filename = path.join(self._root,conf.template)
			template_f = template.load(template_source_filename)
		else
			template_f = template.compile(conf.template_content)
		end
		local filename = path.join(self._root,conf.filename)
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
			root = self._root,
			log = log
		},{__index=_G})
		if conf.config then
			load(conf.config,'generate:config','t',ctx)()
		end
		f:write( template_f(ctx) )
		f:close()
	end

end

local function create_env( cmdargs )
	local env = {
		modules = {}
	}
	local global_env = {
		cmdargs = cmdargs or {}
	}
	local super_env = {}
	for n,v in pairs(Project.env) do
		super_env[n] = function(...)
			v(env,...)
		end
	end
	return setmetatable( global_env, {__index=super_env} ), env
end

function Project.load( root_dir , cmdargs )
	if root_dir then
		root_dir = path.join(fs.pwd(),root_dir)
	else
		root_dir = fs.pwd()
	end
	local load_env,env = create_env( cmdargs )
	local res,err = loadfile(path.join(root_dir,'llae-project.lua'),'bt',load_env)
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

	log.debug('loaded project at',root_dir)
	return Project.new(env,root_dir,cmdargs)
end

return Project