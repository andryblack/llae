local tool = require 'tool'
local class = require  'llae.class'
local uv = require 'uv'
local async = require 'llae.async'


local run = class(tool)
run.descr = 'run script'


function run:exec( args )
	local this = assert(uv.exepath())
	local script = args[2]
	if not script then
		error('need script argument')
	end

	async.run(function()
		local Project = require 'project'
		local prj,err = Project.load( )
		if prj then
			for _,v in ipairs(prj:get_commands() or {}) do
				if v.name == script then
					script = v.script
				end
			end
		end
		
		local run_args = {}
		for k,v in pairs(args) do
			if type(k) == 'string' then
				run_args[k]=v
			end
		end
		run_args[0] = this
		local i = 3
		while args[i] do
			run_args[i-2] = args[i]
			i = i + 1
		end
		_G.args = run_args
		dofile(script)
	end,true)

	
end

return run