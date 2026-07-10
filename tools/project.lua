local class = require 'llae.class'
local modules = require 'modules'
local fs = require 'llae.fs'
local template = require 'llae.template'
local path = require 'llae.path'
local log = require 'llae.log'
local utils = require 'llae.utils'
local llae = require 'llae'
local tool = require 'tool'

---@class Project_config


---@class Project
---@field env table<string, any>
---@field new fun(config:Project_config):Project
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

function Project.env:lock_module( config )
	if (type(config) ~= 'table') or (not config[1]) or (not config[2]) then
		error('need config {name,revision}')
	end
	self.lock_modules = self.lock_modules or {}
	if self.lock_modules[config[1]] then
		error('module ' .. tostring(config[1]) .. ' already locked')
	end
	self.lock_modules[config[1]] = config[2]
end

function Project.env:self_module( name )
	if type(name) ~= 'string' then
		error('module must be string')
	end
	if self.self_module then
		error('self_module already set')
	end
	self.self_module = name
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

function Project.env:global_config( name, value )
	if not self.global_config then
		self.global_config = {[name] = value}
	else
		self.global_config[name] = value
	end
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

function Project.env:exe_command(data)
	if type(data) ~= 'table' then
		error('exe_command must be table')
	end
	if not data.name then
		error('exe_command need name')
	end
	data.project_exe = true
	if not self.commands then
		self.commands = { data }
	else
		table.insert(self.commands,data)
	end
end


function Project.env:print(...)
	log.info('[project]',...)
end

function Project.env:bind_header( filename )
	if not self.bind_headers then
		self.bind_headers = {}
	end
	table.insert(self.bind_headers, { filename = filename })
end

function Project.env:bind_headers( headers )
	if not self.bind_headers then
		self.bind_headers = {  }
	end
	table.insert(self.bind_headers, { dir = headers })
end

local function get_target(cmdargs)
	return  (cmdargs and cmdargs['target-platform']) or os.getenv('LLAE_TARGET_PLATFORM') or llae.get_host_platform()
end

local function add_cmodules(dst,src)
	if src then
		for _,cmod in ipairs(src) do
			if type(cmod) == 'string' then
				local name = string.match(cmod,'C:(.*)')
				if name then
					table.insert(dst,{name=name,func='luaopen_' .. string.gsub(name,'[%.%-]','_'),decl='extern "C" '})
				else
					table.insert(dst,{name=cmod,func='luaopen_' .. string.gsub(cmod,'[%.%-]','_'),decl=""})
				end
			else
				table.insert(dst,{name=cmod[1],func=cmod[2],decl=cmod[3] and 'extern "C" ' or ''})
			end
		end
	end
end

function Project:_init( env , filename )
	log.debug('Project:_init')
	self._env = env or error('env is required')
	self._filename = filename
	self._env.project = self
	self._scripts = {}
	self._metas = {}
	self._modules_locations = {}
	self._modules = {}
	self._modules_list = {}
	self._cmodules = {}
	self._project_cmodules = {}
	self._module_config = env.module_config or {}
	
	if self._env.location then
		self:add_modules_location(path.join(self._env.location,'modules'))
	end
	
	add_cmodules(self._project_cmodules,self._env.cmodules)
	local cmdargs = self._env.cmdargs
	self._dl_dir = (cmdargs and cmdargs['dl-dir']) or os.getenv('LLAE_DL_DIR') or tool.get_llae_path('dl')
	self._target = get_target( cmdargs )
end

function Project:get_dl_dir()
	return self._dl_dir
end

function Project:get_var(name)
	if self._env.__write_env then
		return self._env.__write_env[name]
	end
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

function Project:get_module_lock_revision( name )
	if self._env.lock_modules then
		return self._env.lock_modules[name]
	end
	return nil
end

function Project:get_commands( )
	return self._env.commands
end

function Project:get_exe_ext()
	if self:get_host_platform() == 'windows' then
		return '.exe'
	end
	return ''
end

function Project:get_exe_path()
	return path.join('bin',self._env.project_name .. self:get_exe_ext())
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

function Project:add_module( name , install)
	if self._modules[name] then
		return
	end
	log.debug('add module',name)
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
	add_cmodules(self._cmodules,m:get_cmodules())
	m:load_configs(self._module_config)
end

function Project:add_self_module( name, install )
	log.debug('add self module',name)
	if self._modules[name] then
		log.error('self module already added',name)
		return
	end
	local sm = require 'modules.self'
	local m = assert(sm.load(self,name))
	m:set_root(self:get_root())
	m:set_project(self)
		
	self._modules[name] = m
	
	if m:get_dependencies() then
		for _,v in ipairs(m:get_dependencies()) do
			self:add_module(v, install)
		end
	end
	table.insert(self._modules_list,m)
	add_cmodules(self._cmodules,m:get_cmodules())
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
	if self._env.self_module then
		self:add_self_module(self._env.self_module,install)
	end
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
			end,
			project=self,
			template = template,
			path = path,
			fs = fs,
			log = log,
			table = table,
			string = string,
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
		log.error('required script',file,'from module',m.name)
		log.error('already installed by module',self._scripts[file].name)
		error('script rewrite: ' .. file)
	end
	self._scripts[file] = m
end

function Project:check_meta( file , m )
	if self._metas[file] then
		log.error('required meta',file,'from module',m.name)
		log.error('already installed by module',self._metas[file].name)
		error('meta rewrite: ' .. file)
	end
	self._metas[file] = m
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
	return utils.list_concat(self._cmodules,self._project_cmodules)
end

function Project:get_global_config( config_name )
	return (self._env.global_config or {})[config_name]
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
	local template_source_filename = self:get_llae_path('data','premake5-template.lua')
	local build_root = path.getabsolute(path.join(self:get_root(),'build'))
	local filename = path.join(build_root,'premake5.lua')
	log.info('generate premake5.lua',path.getrelative(template_source_filename,self:get_root()))
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
		utils = utils,
		get_relative = function(dir)
			assert(path.isabsolute(dir))
			return path.getrelative(dir,build_root)
		end
	}))
	f:close()
end

function Project:get_llae_path( ... )
	local m = self:get_module('llae')
	if m then
		return path.join(m:get_location(),m:get_env().dir,...)
	end
	return tool.get_llae_path(self,...)
end

function Project:generate_bindings( )
	local process_bind = require 'cparse.process_bind'
	local processor = process_bind.new()
	processor:define('LLAE_BINDING_GENERATION')
	processor:define('META_OBJECT')

	local dst_dir = path.join(self:get_root(),'build','src')

	local function generate_binding(filename,root)
		local source_filename = path.join(root,filename)
		local result = processor:process_file(source_filename)
		if not next(result.classes) and not next(result.modules) then
			return
		end
		
		log.info('generate binding',filename)

		local header = path.getrelative(path.getabsolute(source_filename),path.getabsolute(dst_dir))
		for n,m in pairs(result.modules) do
			m:add_header(header)
		end
	end

	local all_bind_headers = {}
	for _,conf in ipairs(self._env.bind_headers or {}) do
		conf.root = self:get_root()
		if conf.filename then
			conf.filename = utils.replace_tokens(conf.filename,self._env)
		end
		if conf.dir then
			conf.dir = utils.replace_tokens(conf.dir,self._env)
		end
		table.insert(all_bind_headers,conf)
	end
	for _,m in ipairs(self._modules_list) do
		for _,conf in ipairs(m:get_bind_headers()) do
			conf.root = m:get_location() or self:get_root()
			if conf.filename then
				conf.filename = utils.replace_tokens(conf.filename,m:get_env())
			end
			if conf.dir then
				conf.dir = utils.replace_tokens(conf.dir,m:get_env())
			end
			table.insert(all_bind_headers,conf)
		end
	end

	for _,conf in ipairs(all_bind_headers) do
		assert(conf.root,'root is required')
		if conf.filename then
			generate_binding(conf.filename,conf.root)
		elseif conf.dir then
			log.info('process bind headers dir',conf.dir)
			local dir = path.join(conf.root,conf.dir)
			if not fs.isdir(dir) then
				log.error('bind header dir not found',dir)
			else
				for _,filename in ipairs(fs.scanfiles_r(dir)) do
					local ext = path.extension(filename)
					if ext == 'h' or ext == 'hpp' then
						generate_binding(path.join(conf.dir,filename),conf.root)
					end
				end
			end
		end
	end
	for name,module in pairs(processor:get_modules()) do
		local dst_filename = path.join(self:get_root(),'build','src','gen_module_' .. name .. '.cpp')
		local template_source_filename = self:get_llae_path('data','binding-module-template.cpp')

		fs.mkdir_r(path.dirname(dst_filename))
		fs.unlink(dst_filename)

		log.info('generate module binding',name)

		local f = assert(fs.open(dst_filename,fs.O_WRONLY|fs.O_CREAT))
		f:write(template.render_file(template_source_filename,{
			escape = tostring,
			project=self,
			template = template,
			path = path,
			fs = fs,
			log = log,
			utils = utils,
			headers = {},
			module = module,
		}))
		f:close()
		add_cmodules(self._project_cmodules,{module:get_name()})

		local meta_filename = path.join(self:get_root(),'build','lua-meta', name .. '.lua')
		local template_source_filename = self:get_llae_path('data','binding-meta-template.lua')
		fs.mkdir_r(path.dirname(meta_filename))
		fs.unlink(meta_filename)
		local f = assert(fs.open(meta_filename,fs.O_WRONLY|fs.O_CREAT))
		f:write(template.render_file(template_source_filename,{
			escape = tostring,
			project=self,
			template = template,
			path = path,
			fs = fs,
			log = log,
			utils = utils,
			headers = {},
			module = module,
		}))
		f:close()
	end
end

function Project:write_generated( )
	self:load_modules()
	self:write_premake()

	self:generate_bindings()

	
	
	for _,conf in ipairs(self._env.generate_src or {}) do
		local template_f
		local template_source_filename
		if conf.template then
			template_source_filename = path.join(self:get_root(),conf.template)
			template_f = template.load(template_source_filename)
			template_source_filename = path.getrelative(template_source_filename,self:get_root())
		else
			template_f = template.compile(conf.template_content)
		end
		local filename = path.join(self:get_root(),conf.filename)
		log.info('generate',conf.filename,template_source_filename)
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
			local template_source_filename
			if conf.template then
				template_source_filename = Project.get_path(m:get_location(),conf.template)
				template_f = template.load(template_source_filename)
				template_source_filename = path.getrelative(template_source_filename,self:get_root())
			else
				template_f = template.compile(conf.template_content)
			end
			local filename = path.join(self:get_root(),conf.filename)
			log.info('generate',m:get_name(),conf.filename,template_source_filename)
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

function Project:patch_project( section_begin, section_end, replace_content )
	if not self._filename then
		error('project is not loaded from file')
	end
	local content = tostring(fs.load_file(self._filename))
	local begin_pos = string.find(content,section_begin,1,true)
	if not begin_pos then
		error('not found start marker ' .. tostring(section_begin) .. ' in ' .. self._filename)
	end
	begin_pos = begin_pos + #section_begin
	local end_pos = content:find(section_end,begin_pos,true)
	if not end_pos then
		error('not found end marker ' .. tostring(section_end) .. ' in ' .. self._filename)
	end
	while end_pos > begin_pos do
		if content:sub(end_pos-1,end_pos-1) ~= '\n' then
			end_pos = end_pos - 1
		else
			break
		end
	end
	local new_content = string.sub(content,1,begin_pos-1) ..'\n' .. replace_content .. '\n' .. string.sub(content,end_pos)
	fs.write_file(self._filename,new_content)
end

function Project:lock_modules( modules_dir )
	if not modules_dir then
		modules_dir = path.join(self:get_root(),'modules')
	end
	if not fs.isdir(modules_dir) then
		error('modules dir not found ' .. tostring(modules_dir))
	end
	log.debug('lock modules to ',modules_dir)
	local locked = {}
	for name,module in pairs(self._modules) do
		local lock_config = module:lock(modules_dir)
		if lock_config then
			assert(lock_config.name == name,'invalid module name ' .. tostring(name) .. ' expected ' .. tostring(lock_config.name))
			table.insert(locked,lock_config)
		else
			log.debug('module',name,'is not locked')
		end
	end
	table.sort(locked,function(a,b)
		return a.name < b.name
	end)
	local content = {}
	self._env.lock_modules = {}
	for _,lock in ipairs(locked) do
		log.info('locked module',lock.name,lock.revision,lock.url)
		table.insert(content,string.format('lock_module{%q,%q}',lock.name,lock.revision))
		self._env.lock_modules[lock.name] = lock.revision
	end
	self:patch_project('@lock@','@endlock@',table.concat(content,'\n'))
end

function Project:unlock_modules()
	self._env.lock_modules = nil
	self:patch_project('@lock@','@endlock@','')
end

local function create_env( cmdargs  )
	local env = {
		modules = {},
		args = cmdargs,
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
	env.config = {}
	env.__write_env = write_env
	env.__load_env = load_env
	local filename = path.join(root_dir,'llae-project.lua')
	local res,err = loadfile(filename,'bt',load_env)
	if not res then
		--log.error('failed to load llae-project.lua',err)
		return res,err
	end
	res,err = pcall(res)
	if not res then
		log.error('failed to parse llae-project.lua',err)
		return res,err
	end

	-- validate
	if not env.project_name then
		return nil,'need project name'
	end

	log.debug('loaded project at',root_dir)
	return Project.new(env,filename)
end

return Project