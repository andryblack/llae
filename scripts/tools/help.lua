local tool = require 'tool'
local class = require  'llae.class'

local help = class(tool)
help.descr = 'show usage info'
help.args = {}

function help:exec( args )
	print( 'usage:', args[0], 'command' ,'[arguments]')
	print( 'commands:')
	local commands = require 'commands'
	for _,cmd in ipairs(commands.list) do
		print('',string.format('%s%s%s',cmd.name,string.rep(' ',16-#cmd.name),cmd.descr or ''))
	end
end

return help