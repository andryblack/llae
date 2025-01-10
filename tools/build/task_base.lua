local class = require 'llae.class'
local async = require 'llae.async'
local log = require 'llae.log'

local task_base = class()

function task_base:_init(name)
	self._name = name
	self._ready = async.event.new()
	self._deps = {}
end

function task_base:get_dependencies() 
	return self._deps
end

function task_base:add_depend(dep)
	for _,v in ipairs(self._deps) do
		if v == dep then
			return
		end
	end
	table.insert(self._deps,dep)
end

function task_base:log_start(build)
	log.info(self._name)
end

function task_base:work(build)
	return true
end

function task_base:log_end(build)
end

function task_base:exec(build)
	self:log_start(build)
	local res,err = self:work(build)
	if not res then
		error(err)
	end
	self:log_end(build)
	self._ready:set()
end

function task_base:wait_ready()
	return self._ready:wait()
end

return task_base