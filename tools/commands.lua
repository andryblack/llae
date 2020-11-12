local commands = {}

commands.list = {}
commands.map = {}

local function register( modname )
	--print('register',modname)
	local mod = require( modname )
	if not mod.name then
		mod.name = modname
	end
	table.insert(commands.list, mod )
	commands.map[mod.name] = mod
end

register('install')
register('bootstrap')
register('init_project')
register('help')
register('run')

return commands