local path = require 'llae.path'
local fs = require 'llae.fs'
local utils = require 'llae.utils'
local os = require 'llae.os'
local http = require 'llae.http'

local m = {}

local function exec_cmd(cmd)
	print('CMD:',cmd)
	
	local res,status,code = os.execute(cmd)
	if not res or status ~= 'exit' then
		error(status .. ' ' .. code or 'failed exec ' .. cmd)
	end
	if code ~= 0 then
		error('code ' .. code .. ' ' .. cmd)
	end

end

function m:download_git(url,config)
	print('download_git',self.name,url)
	local dst = path.join(self.location,'src')
	fs.rmdir_r(dst)
	local tag = config.tag or config.branch or 'master'
	local cmd = 'git clone --depth 1 --branch ' .. tag .. ' --single-branch ' .. url .. ' ' .. dst
	exec_cmd(cmd .. ' > ' .. path.join(self.location,'download_git_log.txt'))
end

function m:download(url,file)
	print('download',self.name,url)
	local dst = path.join(self.location,file)
	fs.unlink(dst)

	local req = http.createRequest{
		method = 'GET',
		url = url,
		headers = {
			['Accept'] = '*/*'
		}
	}

	local resp = assert(req:exec())
	local code = resp:get_code()
	if code ~= 200 then
		resp:close()
		error(code .. ':' .. resp:get_message())
	end
	local f = assert(fs.open(dst,fs.O_WRONLY|fs.O_CREAT))
	while true do
		local ch,err = resp:read()
		if not ch then
			if err then
				error(err)
			end
			break
		end
		f:write(ch)
	end
end

function m:shell( text )
	print( 'shell' )
	local script = path.join(self.location,'temp.sh')
	fs.unlink(script)
	local f = io.open(script,'w')
	f:write('#!/bin/sh\n')
	f:write('LLAE_PROJECT_ROOT=' .. self.root .. '\n')
	f:write('cd $(dirname $0)\n')
	f:write(text)
	f:close()
	local cmd = '/bin/sh "' .. script .. '"' 
	exec_cmd(cmd .. ' > ' .. path.join(self.location,'shell_script_log.txt'))
end

function m:install_bin( fn )
	local src = path.join(self.location,fn)
	local dst = path.join(self.root,'bin',path.basename(fn))
	print('install',src,'->',dst)
	assert(fs.copyfile(src,dst))
end

function m:install_files( files )
	for to,from in pairs(files) do
		local src = path.join(self.location,from)
		local dst = path.join(self.root,to)
		print('install',src,'->',dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:preprocess( config )
	local src_file = path.join(self.location,config.src)
	local dst_file = path.join(self.root,config.dst)

	local data = {}
	local uncomment = config.uncomment or {}
	local comment = config.comment or {}
	local replace = config.replace or {}
	for line in io.lines(src_file) do 
		--print('process line',line)
		local d,o = string.match(line,'^//#define%s+([A-Z_]+)(.*)$')
		if d then
			if uncomment[d] then
				print('uncomment',d)
				line = '#define ' .. d .. o
			end
		else
			d,o = string.match(line,'^#%s*define%s+([A-Z_]+)(.*)$')
			if d and comment[d] then
				print('comment',d)
				line = '//#define ' .. d .. o
			elseif d and replace[d] then
				print('replace',d)
				line = '#define ' .. d .. ' ' .. replace[d]
			end
		end
		table.insert(data,line)
	end
	fs.write_file(dst_file,table.concat(data,'\n'))
end

local _M = {}

function _M.create_env(  )
	local env = {}
	local super_env = {}
	for n,v in pairs(m) do
		super_env[n] = function(...)
			v(env,...)
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

function _M.install( env , root )
	
	print('install module',env.name,env.version)
	env.root = root or env.root or utils.replace_env(fs.pwd())
	os.setenv('LLAE_PROJECT_ROOT',env.root)
	env.location = path.join(env.root,'build','modules', env.name)
	fs.mkdir(env.location)
		
	
	env.install()
end

function _M.install_file( filename, root )
	local env = _M.loadfile( filename )
	_M.install(env)
	local dst = path.join(root,'modules',env.name .. '.lua')
	if dst ~= filename then
		print('install',filename,'->',dst)
		fs.unlink(dst)
		assert(fs.copyfile(filename,dst))
	end
end

function _M.get( root, modname )
	local fn = path.join(root,'modules',modname .. '.lua')
	if fs.isfile(fn) then
		local mod =  _M.loadfile(fn)
		mod.root = root
		mod.location = path.join(mod.root,'build','modules', mod.name)
		return mod
	end
	error('not found module ' .. modname)
end

return _M