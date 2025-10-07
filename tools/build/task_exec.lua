local class = require 'llae.class'
local async = require 'llae.async'
local uv = require 'llae.uv'
local fs = require 'llae.fs'

local task = class(require 'build.task_base')

function task:_init(name,exe,args,cwd)
	task.baseclass._init(self,name)
	self._exe = exe
	self._args = args
	self._cwd = cwd
end

local function redirect_pipe(pipe,logbuf,level)
	async.run(function()
		local d = ''
		while true do
			local ch,err = pipe:read()
			if not ch then
				if err then
					error(err)
				else
					table.insert(logbuf,{level,d})
					break
				end
			else
				d = d .. ch
				local el = d:find('\n',1,true)
				if el then
					table.insert(logbuf,{level,d:sub(1,el-1)})
					d = d:sub(el+1)
				end
			end
		end
	end)
end

local exe_cache = {}
local function get_exename(exe)
	local res = exe_cache[exe]
	if res then
		return res
	end
	res = fs.find_exe(exe)
	exe_cache[exe] = res
	return res
end


function task:work(build)
	self._log = {}
	local rpipe = uv.pipe.new(1)
	local epipe = uv.pipe.new(1)
	local p = assert(uv.process.spawn{
		file = get_exename(self._exe),
		args = args,
		--env = env,
		cwd = self._cwd,
		streams = {
			{uv.process.IGNORE},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,rpipe},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,epipe}
		}
	})
	redirect_pipe(rpipe,self._log,'info')
	redirect_pipe(epipe,self._log,'error')
	local err
	local code,sig = p:wait_exit()
	if code ~= 0 or sig ~= 0 then
		err = string.format('process code:%d sig:%d',code,sig)
	end
	rpipe:close()
	epipe:close()
	return not err,err
end


return task