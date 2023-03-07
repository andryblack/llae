
local fs = require 'llae.fs'
local async = require 'llae.async'


local cmdargs = args

package.path = '../scripts/?.lua;?.lua'

if cmdargs.ssldebug then
	local ssl = require 'ssl'
	ssl.ctx.set_debug_threshold(tonumber(cmdargs.ssldebug))
end

local this = assert(fs.exepath())
local script = cmdargs[1]
if not script then
	error('need script argument')
end

async.run(function()
	local run_args = {}
	for k,v in pairs(cmdargs) do
		if type(k) == 'string' then
			run_args[k]=v
		end
	end
	run_args[0] = this
	local i = 3
	while cmdargs[i] do
		run_args[i-2] = cmdargs[i]
		i = i + 1
	end
	_G.args = run_args
	dofile(script)
end,true)

