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

register('help')
register('version')
register('install')
register('bootstrap')
register('init_project')
register('run')
register('http_server')
register('upgrade')

return commands