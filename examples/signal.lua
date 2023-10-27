package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local log = require 'llae.log'
local async = require 'llae.async'
local uv = require 'llae.uv'

async.run(function()
	while true do
		async.pause(1000)
		log.info('ping')
	end
end)


llae.cancel_sigint()

uv.signal.oneshot(2,function(s)
	log.error('signint','stop',s)
	llae.stop(s)
end)