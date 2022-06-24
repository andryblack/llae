package.path = package.path .. ';scripts/?.lua'

local utils = require 'llae.utils'
local log = require 'llae.log'
local pgsql = require 'db.pgsql'
local json = require 'llae.json'

local uv = require 'uv'

local cin = assert(uv.tty.new(0))

local args = utils.parse_args(_G.args)

utils.run(function()
	local pg = pgsql.new()
	assert(pg:connect())
	while true do
		log.info('ready')
		local line = assert(cin:read())
		line = line:gsub('\n','')
		log.info('>line')
		local res,err = pg:query(line)
		--log.info(res,err)
		log.info('<',json.encode(res),json.encode(err))
	end
end,true)