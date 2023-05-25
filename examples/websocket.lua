package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local websocket = require 'net.websocket'
local log = require 'llae.log'
local fs = require 'llae.fs'
local json = require 'json'
local uv = require 'uv'
local async = require 'llae.async'


local th = coroutine.create(function()
	
	local ws = websocket.createClient{
		url = 'ws://127.0.0.1:8080',
	}

	async.run(function()
		local len = 1
		while true do
			async.pause(1000)
			ws:text(string.rep('PING',len))
			len = len + 1
			if len > 32 then
				len = 1
			end
		end
	end)

	assert(ws:start(function(msg,err)
		if msg then
			log.info('RX:',msg)
		else
			log.error(err)
		end
	end))

	ws:close()
	print('finished request')

end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

