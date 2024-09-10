local class = require 'llae.class'
local llae = require 'llae'
local log = require 'llae.log'
local uv = require 'uv'

local _M = {}

_M.pause = uv.pause

function _M.resume( th )
	local res, err = coroutine.resume( th )
	if not res then
		log.error('failed resume:',err,debug.traceback(th),'frome',debug.traceback())
		error(err or 'unknown')
	end
end

function _M.run( fn , handle_error )
	local th = coroutine.create( handle_error and function() 
		local res,err = xpcall(fn,debug.traceback)
		if not res then
			error(err or 'unknown')
		end
	end or fn )
	_M.resume(th)
end

local lock = class()

function lock:_init()
	self._wait = {}
end

function lock:report_unlock()
	local u = table.remove(self._wait,1)
	if u then
		_M.resume(u)
	end
end

function lock:wait_unlock()
	local c = coroutine.running()
	table.insert(self._wait,c)
	coroutine.yield()
end

function lock:lock()
	while self._locked do
		self:wait_unlock()
	end
	self._locked = true
end

function lock:unlock()
	self._locked = false
	self:report_unlock()
end

_M.lock = lock


local event = class()

function event:_init()
	self._wait = {}
	self._set = false
end

function event:clear()
	self._set = false
end

function event:set()
	self._set = true
	while self._set do
		local u = table.remove(self._wait,1)
		if u then
			_M.resume(u)
		else
			return
		end
	end
end

function event:wait()
	if self._set then
		return
	end
	local c = coroutine.running()
	table.insert(self._wait,c)
	coroutine.yield()
end

_M.event = event

return _M
