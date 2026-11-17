package.path = package.path .. ';scripts/?.lua'

local async = require 'llae.async'
local log = require 'llae.log'

local uv = require 'llae.uv'


local udp = uv.udp.new()

assert(udp:bind('127.0.0.1',8888,uv.udp.REUSEADDR))
log.info('start UDP echo on',8888)

async.run(function()
	while true do
		local data,from = udp:recv()
		if not data then
			error(from)
		end
		log.info('recv',data,from)
		udp:send('echo:'..data,from)
	end
end)