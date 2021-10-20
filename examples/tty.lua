package.path = package.path .. ';scripts/?.lua'

local utils = require 'llae.utils'
local log = require 'llae.log'

local uv = require 'uv'

local cerr = assert(uv.tty.new(2))
local cin = assert(uv.tty.new(0))
local cout = assert(uv.tty.new(1))

utils.run(function()
	cerr:write('test on cerr\n')
	cout:write('test on cout\n')
	log.info('enter any stting')
	local d = cin:read()
	log.info('readed from cin:',d)

end)