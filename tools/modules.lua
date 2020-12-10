local path = require 'llae.path'
local fs = require 'llae.fs'
local utils = require 'llae.utils'
local os = require 'llae.os'
local http = require 'net.http'
local log = require 'llae.log'
local untar = require 'untar'


local m = {}

local function exec_cmd(cmd)
	log.info('CMD:',cmd)
	
	local res,status,code = os.execute(cmd)
	if not res or status ~= 'exit' then
		error(status .. ' ' .. code or 'failed exec ' .. cmd)
	end
	if code ~= 0 then
		error('code ' .. code .. ' ' .. cmd)
	end

end

function m:download_git(url,config)
	log.info('download_git',self.name,url)
	local dst = path.join(self.location,config.dir or 'src')
	fs.rmdir_r(dst)
	local tag = config.tag or config.branch or 'master'
	local cmd = 'git clone --depth 1 --branch ' .. tag .. ' --single-branch ' .. url .. ' ' .. dst
	exec_cmd(cmd .. ' > ' .. path.join(self.location,'download_git_log.txt') .. ' 2>&1')
end

function m:download(url,file)
	log.info('download',self.name,url)
	local dst = path.join(self.location,file)
	fs.unlink(dst)

	local uri = (require 'net.url').parse(url)
	if uri.scheme == 'ftp' then
		local ftp = (require 'net.ftp').new()
		assert(ftp:connect(uri.host,uri.port))
		local loaded = 0
		local p = log.progress()

		ftp:getfile(uri.path,dst,function(ch,total)
			loaded = loaded + #ch
			p:update(loaded,total)
		end)
	else
	
		local req = http.createRequest{
			method = 'GET',
			url = url,
			headers = {
				['Accept'] = '*/*',
				['Accept-Encoding'] = 'identity'
			}
		}

		local resp = assert(req:exec())
		local code = resp:get_code()
		if code ~= 200 then
			resp:close()
			error(code .. ':' .. resp:get_message())
		end
		local f = assert(fs.open(dst,fs.O_WRONLY|fs.O_CREAT))
		local loaded = 0
		local total = tonumber(resp:get_header('Content-Length'))
		--log.debug('total:',total)
		local p = log.progress()
		while true do
			local ch,err = resp:read()
			if not ch then
				if err then
					error(err)
				end
				p:close()
				break
			end
			f:write(ch)
			loaded = loaded + #ch
			p:update(loaded,total)
		end
		f:close()
	end
end

function m:unpack_tgz( file , todir )
	local src = path.join(self.location,file)
	local dst = todir and path.join(self.location,todir) or self.location
	log.info('unpack',file)
	untar.unpack_tgz(src,dst)
end

function m:shell( text )
	local script = path.join(self.location,'temp.sh')
	fs.unlink(script)
	local f = io.open(script,'w')
	local template = require 'llae.template'
	f:write('#!/bin/sh\n')
	f:write('LLAE_PROJECT_ROOT=' .. self.root .. '\n')
	f:write('PATH=$LLAE_PROJECT_ROOT/bin:$PATH' .. '\n')
	f:write('cd $(dirname $0)\n')
	f:write(template.compile(text,{env={escape=tostring}})(self))
	f:close()
	local cmd = '/bin/sh "' .. script .. '"' 
	exec_cmd(cmd .. ' > ' .. path.join(self.location,'shell_script_log.txt'))
end

function m:install_bin( fn )
	local src = path.join(self.location,fn)
	local dst = path.join(self.root,'bin',path.basename(fn))
	log.debug('install',src,'->',dst)
	assert(fs.copyfile(src,dst))
end

function m:install_files( files )
	for to,from in pairs(files) do
		local src = path.join(self.location,from)
		local dst = path.join(self.root,to)
		fs.mkdir(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.mkdir_r(path.dirname(dst))
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:write_file( filename , content)
	local dst = path.join(self.root,filename)
	fs.mkdir(path.dirname(dst))
	fs.unlink(dst)
	fs.write_file(dst,content)
end

function m:move_files( files )
	for to,from in pairs(files) do
		local src = path.join(self.location,from)
		local dst = path.join(self.root,to)
		fs.mkdir(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.rename(src,dst))
	end
end

function m:install_scripts( dir )
	local src = path.join(self.location,dir)
	local files,err = fs.scanfiles_r(src)
	if not files then
		error(err)
	end
	for _,f in ipairs(files) do
		self._project:check_script(f,self)
		local src = path.join(self.location,dir,f)
		local dst = self.tosystem and path.join(self.root,'scripts',f) or 
			path.join(self.root,'build','scripts',f)
		fs.mkdir_r(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end


function m:foreach_file( dir )
	local src = path.join(self.location,dir)
	local files,err = fs.scandir(src)
	if not files then
		error(err)
	end
	local idx = 0
	return function(files)
		while true do		
			idx = idx + 1
			local f = files[idx]
			if not f then
				return nil
			end
			if f.isfile then
				return f.name
			end
		end
	end, files
end


function m:preprocess( config )
	local src_file = path.join(self.location,config.src)
	local dst_file = path.join(self.root,config.dst)

	local data = {}
	local uncomment = config.uncomment or {}
	local comment = config.comment or {}
	local replace = config.replace or {}
	local replace_line = config.replace_line or {}
	local insert_before = config.insert_before or {}
	for line in io.lines(src_file) do 
		--print('process line',line)
		local d,o = string.match(line,'^//#define%s+([A-Z_]+)(.*)$')
		if d then
			if uncomment[d] then
				line = '#define ' .. d .. o
			end
		else
			d,o = string.match(line,'^#%s*define%s+([A-Z_]+)(.*)$')
			if d and comment[d] then
				line = '//#define ' .. d .. o
			elseif d and replace[d] then
				line = '#define ' .. d .. ' ' .. replace[d]
			end
		end
		local pre = insert_before[line]
		if pre then
			table.insert(data,pre)
		end
		table.insert(data,replace_line[line] or line)
	end
	fs.write_file(dst_file,table.concat(data,'\n'))
	if config.remove_src then
		fs.unlink(src_file)
	end
end

local _M = {}

function _M.create_env(  )
	local env = {}
	local super_env = {}
	for n,v in pairs(m) do
		super_env[n] = function(...)
			return v(env,...)
		end
	end
	setmetatable( env, {__index=setmetatable(super_env,{__index=_G})} )
	return env
end

function _M.check_module( env, name )
	local res,err = pcall(function()
		if not env.name then
			error('need name')
		end
		if type(env.install) ~= 'function' then
			error('need install function')
		end
	end)
	if not res then
		error(err .. ' at ' .. name)
	end
end

function _M.loadfile( filename )
	local env = _M.create_env()
	assert(loadfile(filename,'bt',env))()
	_M.check_module(env,'file:' .. filename)
	return env
end

function _M.load( data , name)
	local env = _M.create_env()
	assert(load(data,name,'bt',env))()
	_M.check_module(env,'file:' .. filename)
	return env
end

function _M.install( env , root , tosystem)
	
	log.info('install module',env.name,env.version)
	env.root = root or env.root or utils.replace_env(fs.pwd())
	os.setenv('LLAE_PROJECT_ROOT',env.root)
	env.location = path.join(env.root,'build','modules', env.name)
	env.tosystem = tosystem
	fs.rmdir_r(env.location)
	fs.mkdir_r(env.location)
		
	env.install(tosystem)
end

function _M.install_file( filename, root )
	local env = _M.loadfile( filename )
	_M.install(env)
	local dst = path.getabsolute(path.join(root,'modules',env.name .. '.lua'))
	local src = path.getabsolute(filename)
	if src ~= dst then
		log.info('install',src,'->',dst)
		fs.mkdir_r(path.dirname(dst))
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function _M.get( locations, modname )
	local mod
	local root
	for _,v in ipairs(locations) do
		local fn = path.join(v,modname .. '.lua')
		if fs.isfile(fn) then
			mod = _M.loadfile( fn )
			mod.source = fn
			break
		else
			log.debug('not found module at',fn)
		end
	end
	if not mod then
		local fn = path.join(path.dirname(fs.exepath()),'..','modules',modname .. '.lua')
		if fs.isfile(fn) then
			mod =  _M.loadfile(fn)
			mod.source = fn
		end
	end

	if mod then
		return mod
	end
	log.error('not found module ' , modname)
	error('not found module ' .. modname)
end

return _M