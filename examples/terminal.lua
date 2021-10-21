package.path = package.path .. ';scripts/?.lua'

local utils = require 'llae.utils'
local log = require 'llae.log'
local serial = require 'posix.serial'

local uv = require 'uv'

local cin = assert(uv.tty.new(0))
local cout = assert(uv.tty.new(0))


local path = '/dev/tty.usbmodem00000000001C1'

utils.run(function()
	local s = assert(serial.open(path,{baudrate=115200}))
	log.info('opened serial connection to',path)
	utils.run(function()
		while true do
			local line = assert(cin:read())
			assert(s:write(line))
		end
	end)
	log.info('start reading')
	while true do
		local d = assert(s:read())
		cout:write(d)
		--log.info('>',d)
	end
end)