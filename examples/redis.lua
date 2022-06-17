package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local log = require 'llae.log'
local redis = require 'db.redis'
local utils = require 'llae.utils'
local json = require 'json'
local uv = require 'llae.uv'

local function create_redis()
	local rds = redis.new()
	assert(rds:connect('127.0.0.1',6379))
	return rds
end

utils.run(function()
	local res,err = pcall(function()
		local rds = create_redis()

		log.info('SET:',assert(rds:set('test.a.1',100)))
		log.info('INCR:',assert(rds:incr('test.a.1')))
		log.info('GET:',rds:get('test.a.1'))

		log.info('HGETALL:',rds:hgetall('test.not.exist'))
		log.info('HGETALL:',rds:hgetall('test.a.1'))

		local r = assert(rds:scan("0",'MATCH','test.*'))
		log.info(json.encode(r))
		while true do
			for _,v in ipairs(r[2]) do
				log.info('SCAN:',v)
			end
			if r[1] == "0" then
				break
			end
			r = assert(rds:scan(r[1],'MATCH','test.*'))
		end
	end)
	if not res then
		print('failed exec thread',err)
		error(err)
	end
end)

-- pub
utils.run(function()
	uv.pause(500)
	local rds = create_redis()
	for i = 1,20 do
		rds:publish('ping','msg'..i)
		uv.pause(500)
	end
	uv.pause(1500)
	rds:publish('exit','')
end)

-- sub
utils.run(function()
	local rds = create_redis()
	log.info('subscribe')
	assert(rds:subscribe('ping',function(ch,msg)
		log.info('SUB:',ch,msg)
	end))

	assert(rds:subscribe('exit',function(ch,msg)
		log.info('SUB:',ch,msg)
		assert(rds:unsubscribe())
	end))
	log.info('subscribed')
end)
