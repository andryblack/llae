local commands = {}

commands.list = {}
commands.map = {}

local function register( modname )
	print('register',modname)
	local mod = require( modname )
	mod.name = modname
	table.insert(commands.list, mod )
	commands.map[modname] = mod
end

register('install')
register('bootstrap')

return commands