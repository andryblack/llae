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

function Project:_init( env , root)
	self._env = env
	self._root = root
	self._scripts = {}
	self._modules_locations = {}
	self._modules = {}
	self._modules_list = {}
	if root then
		self:add_modules_location(path.join(root,'modules'))
	end
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

function Project:add_modules_location( loc )
	log.debug('add modules location',loc)
	table.insert(self._modules_locations,loc)
end
function Project:add_module( name )
	if self._modules[name] then
		return
	end
	local m = modules.get(self._modules_locations,name)
	
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

function Project.load( root_dir )
	if root_dir then
		root_dir = path.join(fs.pwd(),root_dir)
	else
		root_dir = fs.pwd()
	end
	local load_env,env = create_env( )
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
	return Project.new(env,root_dir)
end

return Project