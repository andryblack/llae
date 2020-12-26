package.path = package.path .. ';scripts/?.lua'

local utils = require 'llae.utils'
local log = require 'llae.log'

local uv = require 'uv'


local udp = uv.udp.new()

assert(udp:bind('127.0.0.1',8888))
log.info('start UDP echo on',8888)

utils.run(function()
	while true do
		local data,err,addr,port = udp:recv()
		if not data then
			error(err)
		end
		log.info('recv',data,err,addr,port)
		udp:send('echo:'..data,addr,port) 
	end
end)