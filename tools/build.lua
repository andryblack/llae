local class = require 'llae.class'
local utils = require 'llae.utils'
local async = require 'llae.async'

local task_base = class()

function task_base:_init(name)
	self._name = name
	self._ready = async.event.new()
end

function task_base:get_dependencies() 
	return nil
end

function task_base:exec(build)

	self._ready:set()
end

function task_base:wait_ready()
	return self._ready:wait()
end

local build = class()

function build:_init(config)
	self._threads = config.threads or 1
end

function build:exec()
	local tasks = {}
	self._target:collect_tasks(tasks)
	for i=1,self._threads do
		async.run(function( )
			local task = table.remove(tasks)
			if not task then
				return
			end
			local deps = task:get_dependencies()
			if deps then
				for _,dep in ipairs( deps ) do
					dep:wait_ready()
				end
			end
			task:exec()
		end)
	end
end

return build