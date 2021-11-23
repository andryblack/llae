package.path = package.path .. ';scripts/?.lua'

local uv = require 'uv'
local log = require 'llae.log'

local utils = require 'llae.utils'

utils.run(function()
	for i=1,10 do
		uv.pause(1000)
		log.info('pause',i)
	end
end)