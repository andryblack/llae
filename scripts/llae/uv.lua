local uv = require 'uv'
local class = require 'llae.class'
local _M = setmetatable({},{__index=uv})

local lock = class()

function lock:_init()
	self._wait = {}
end

function lock:report_unlock()
	local u = table.remove(self._wait,1)
	if u then
		coroutine.resume(u)
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

return _M