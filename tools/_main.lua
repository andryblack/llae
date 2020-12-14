
local uv = require 'uv'
local utils = require 'llae.utils'

local commands = require 'commands'

local cmdargs = utils.parse_args(...)
local root = cmdargs.root
if root then
	print('use scripts at',root)
	table.remove(package.searchers,1)
	local store_packages = {
		'json','llae','uv','ssl','archive'
	}
	local store = { }
	for _,v in ipairs(store_packages) do
		store[v] = package.loaded[v]
	end
	utils.clear_table(package.loaded)
	for k,v in pairs(store) do
		package.loaded[k]=v
	end
	package.path = root .. '/scripts/?.lua;' .. root .. '/tools/?.lua'
	commands = require 'commands'
	local tool = require 'tool'
	tool.set_root(root)
end
if cmdargs.verbose then
	local log = require 'llae.log'
	log.set_verbose(true)
end
if cmdargs.ssldebug then
	local ssl = require 'ssl'
	ssl.ctx.set_debug_threshold(tonumber(cmdargs.ssldebug))
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

