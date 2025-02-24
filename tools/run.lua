local tool = require 'tool'
local class = require  'llae.class'
local fs = require 'llae.fs'
local async = require 'llae.async'


local run = class(tool)
run.descr = 'run script'


function run:exec( args )
	local this = assert(fs.exepath())
	local script = args[2]
	if not script then
		error('need script argument')
	end

	async.run(function()
		local Project = require 'project'
		local prj,err = Project.load( )
		local add_args 
		local project_exe
		if prj then
			for _,v in ipairs(prj:get_commands() or {}) do
				if v.name == script then
					script = v.script
					add_args = v.args
					if v.project_exe then
						project_exe = prj:get_exe_path()
					end
					break
				end
			end
		end
		
		local run_args = {}
		for k,v in pairs(args) do
			if type(k) == 'string' then
				if (type(v) == 'boolean' and v) then
					table.insert(run_args,'--'..k)
				else
					table.insert(run_args,'--'..k .. '=' .. v)
				end
			end
		end
		local i = 3
		while args[i] do
			table.insert(run_args,args[i])
			i = i + 1
		end
		if add_args then
			for _,v in ipairs(add_args) do
				table.insert(run_args,v)
			end
		end
		if not project_exe then
			if not script then
				error('need script argument')
			end
			local log = require 'llae.log'
			local utils = require 'llae.utils'
			run_args[0] = this
			_G.args = utils.parse_args(run_args)
			log.debug('run script:',script)
			dofile(script)
		else
			local log = require 'llae.log'
			local cmd = project_exe .. ' ' .. table.concat(run_args,' ')
			log.debug('run:',cmd)
			os.execute(cmd)
		end
	end,true)

	
end

return run