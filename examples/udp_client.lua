package.path = package.path .. ';scripts/?.lua'

local async = require 'llae.async'
local log = require 'llae.log'

local uv = require 'uv'


local udp = uv.udp.new()

async.run(function()
	for i=1,10 do
		assert(udp:send('msg'..i,'127.0.0.1',8888))
		log.info(udp:recv())
	end
end)