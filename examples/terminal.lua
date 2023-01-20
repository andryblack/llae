package.path = package.path .. ';scripts/?.lua'

local utils = require 'llae.utils'
local log = require 'llae.log'

local serial = require 'posix.serial'

local uv = require 'uv'

local cin = assert(uv.tty.new(0))
local cout = assert(uv.tty.new(0))

local args = utils.parse_args(_G.args)
--log.info(args[1],args[2],args[3],args[4])
local path = args.device or args[1]
if not path then
	log.error('need device argument')
	return
end
local baudrate = tonumber(args.baudrate or args[2])

utils.run(function()
	local s = assert(serial.open(path,{baudrate=baudrate or 115200}),'failed open serial ' .. tostring(path))
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