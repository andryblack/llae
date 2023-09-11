package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local log = require 'llae.log'
local redis = require 'db.redis'
local async = require 'llae.async'
local json = require 'json'

local function create_redis()
	local rds = redis.new()
	assert(rds:connect('127.0.0.1',6379))
	return rds
end

async.run(function()
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

		local requests = {}
		for i=1,100 do
			table.insert(requests,rds.encode('INCR','foo'))
		end
		table.insert(requests,rds.encode('HGETALL','foo'))
		local responses = assert(rds:pipelining(requests))
		for _,r in ipairs(responses) do
			log.info(table.unpack(r))
		end

		rds:close()
	end)
	if not res then
		print('failed exec thread',err)
		error(err)
	end
end)


-- -- pub
async.run(function()
	async.pause(500)
	local rds = create_redis()
	for i = 1,20 do
		log.info('PING')
		rds:publish('ping','msg'..i)
		async.pause(500)
	end
	async.pause(1500)
	rds:publish('exit','')
	rds:close()
	log.info('pub end')
end)

-- sub
async.run(function()
	local rds = create_redis()
	log.info('subscribe')
	assert(rds:subscribe('ping',function(ch,msg)
		log.info('SUB:',ch,msg)
	end))

	assert(rds:subscribe('exit',function(ch,msg)
		log.info('SUB:',ch,msg)
		log.info('unsubscribe, close')
		async.run(function()
			assert(rds:unsubscribe())
			rds:close()
		end)
	end))
	log.info('subscribed')
end)
