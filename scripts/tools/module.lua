local path = require 'llae.path'
local fs = require 'llae.fs'
local utils = require 'llae.utils'
local os = require 'llae.os'

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
	local cmd = 'git clone -v --depth 1 --branch ' .. tag .. ' --single-branch ' .. url .. ' ' .. dst
	exec_cmd(cmd)
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
	exec_cmd(cmd)
end

function m:install_bin( fn )
	local src = path.join(self.location,fn)
	local dst = path.join(self.root,'bin',path.basename(fn))
	print('install',src,'->',dst)
	assert(fs.copyfile(src,dst))
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
	setmetatable( env, {__index=super_env} )
	return env
end

function _M.loadfile( filename )
	local env = _M.create_env()
	assert(loadfile(filename,'bt',env))()
	return env
end

function _M.install_file( filename, root )
	local env = _M.loadfile( filename )
	print('install module',env.name,env.version)
	env.root = root or utils.replace_env(fs.pwd())
	os.setenv('LLAE_PROJECT_ROOT',env.root)
	env.location = path.join(env.root,'build','modules',env.modname or env.name)
	fs.mkdir(env.location)
		
	
	env.install()
end

return _M