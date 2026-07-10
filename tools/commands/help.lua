local tool = require 'tool'
local class = require  'llae.class'

local help = class(tool)
help.descr = 'show usage info'
help.args = {
	{'','command', false },
}

function help:exec( args )
	local commands = require 'commands'
	if args[2] then
		local cmd = commands.map[args[2]]
		if not cmd then
			print('unknown command',args[2])
			return
		end
		local required = {}
		for _,arg in ipairs(cmd.args or {}) do

		end
		print( 'usage:', args[0], args[2] ,'[arguments]')
		for _,arg in ipairs(cmd.args or {}) do
			local switch = arg[1]
			print('',string.format('%s%s',switch,string.rep(' ',16-#switch),arg[2] or ''))
		end
	else
		print( 'usage:', args[0], 'command' ,'[arguments]')
		print( 'commands:')
		for _,cmd in ipairs(commands.list) do
			print('',string.format('%s%s%s',cmd.name,string.rep(' ',16-#cmd.name),cmd.descr or ''))
		end
	end
end

return help