local class = require 'llae.class'
local llae = require 'llae'
local log = require 'llae.log'
local uv = require 'uv'

---The async module provides functionality for asynchronous programming using coroutines.
---It includes utilities for coroutine management, locks for synchronization, and event handling.
---@class llae.async
local _M = {}

--- Pauses the event loop. This is a direct binding to uv.pause from the libuv library.
_M.pause = uv.pause

--- Resumes a coroutine and handles any errors that occur during execution.
--- Throws an error if the coroutine fails to resume, with detailed stack trace.
---@param th thread The coroutine to resume
function _M.resume( th )
	local res, err = coroutine.resume( th )
	if not res then
		log.error('failed resume:',err,debug.traceback(th),'frome',debug.traceback())
		error(err or 'unknown')
	end
end

---@class llae.async.thread
---@field new fun(thread):llae.async.thread
local thread = class(nil,'llae.async.thread')

function thread:_init(th)
	self._th = th
end

function thread:join()
	while coroutine.status(self._th) ~= 'dead' do
		uv.pause(100)
	end
end

--- Creates and starts a new coroutine to run the given function.
---@param fn function The function to run in a new coroutine
---@param handle_error boolean? If true, wraps the function in error handling code
---@return llae.async.thread
function _M.run( fn , handle_error )
	local th = coroutine.create( handle_error and function() 
		---@type boolean,string?
		local res,err = xpcall(fn,debug.traceback)
		if not res then
			error(err or 'unknown')
		end
	end or fn )
	_M.resume(th)
	return thread.new(th)
end

---@class llae.lock
---The Lock class provides mutual exclusion functionality for coroutines.
---@field new fun():llae.lock
local lock = class(nil,'llae.lock')

function lock:_init()
	self._wait = {}
end

--- Internal method that resumes the next waiting coroutine.
function lock:_report_unlock()
	local u = table.remove(self._wait,1)
	if u then
		self._locked = u
		uv.resume_delayed(u)
	end
end

--- Waits for the lock to be released. This is used internally by the lock method.
function lock:_wait_unlock()
	local c = coroutine.running()
	table.insert(self._wait,c)
	coroutine.yield()
end

--- Acquires the lock. If the lock is already held, waits until it's released.
function lock:lock()
	local c = coroutine.running()
	if self._locked then
		if self._locked == c then
			error('lock called by a coroutine that already holds the lock')
		end
		self:_wait_unlock()
	else
		self._locked = c
	end
end


--- Releases the lock and wakes up one waiting coroutine if any.
function lock:unlock()
	local c = coroutine.running()
	if self._locked ~= c then
		error('unlock called by a coroutine that does not hold the lock')
	end
	self._locked = nil
	self:_report_unlock()
end

_M.lock = lock

---@class llae.event
---The Event class provides a way to synchronize coroutines using events.
---@field new fun():llae.event
local event = class(nil,'llae.event')

function event:_init()
	self._wait = {}
	self._set = false
end

--- Clears the event state.
function event:clear()
	self._set = false
end

--- Sets the event and wakes up all waiting coroutines.
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

--- Waits for the event to be set. If the event is already set, returns immediately.
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
