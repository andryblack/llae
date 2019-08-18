
local llae = require 'llae'
local utils = require 'llae.utils'


local commands = require 'commands'

local function main( args )
	local cmdargs = utils.parse_args(args)
	local scripts = cmdargs.scripts
	if scripts then
		print('use scripts at',scripts)
		table.remove(package.searchers,1)
		local store_packages = {
			'llae','llae.file','llae.json','llae.crypto'
		}
		local store = { }
		for _,v in ipairs(store_packages) do
			store[v] = package.loaded[v]
		end
		utils.clear_table(package.loaded)
		for k,v in pairs(store) do
			package.loaded[k]=v
		end
		package.path = scripts .. '/?.lua;' .. scripts .. '/tools/?.lua'
		commands = require 'commands'
		
	end
	local cmdname = cmdargs[1]
	if not cmdname then
		cmdname = 'help'
	end
	local cmd = commands.map[ cmdname ]
	if not cmd then
		print( 'unknown command', cmdname )
		return
	end
	cmd:exec( cmdargs )

	llae.set_handler()
	llae.run()

end

return main

