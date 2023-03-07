
local utils = require 'llae.utils'


local cmdargs = utils.parse_args(...)
local root = cmdargs.root
if root then
	print('use scripts at',root)
	table.remove(package.searchers,1)
	package.loaded['llae.utils'] = nil
	package.path = root .. '/scripts/?.lua;' .. root .. '/tools/?.lua'
	local tool = require 'tool'
	utils = require 'llae.utils'
	tool.set_root(root)
end

local commands = require 'commands'

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

