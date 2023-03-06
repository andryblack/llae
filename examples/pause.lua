package.path = package.path .. ';scripts/?.lua'

local uv = require 'uv'
local log = require 'llae.log'

local async = require 'llae.async'

async.run(function()
	for i=1,10 do
		uv.pause(1000)
		log.info('pause',i)
	end
end)


do
	local d = 1
	local tmr = uv.timer.new()
	tmr:start(function(t)
		log.info('on_timer',d)
		d = d + 1
		if d > 10 then
			t:stop()
		end
	end,3000,200)
end