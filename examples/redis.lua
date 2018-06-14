package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local redis = require 'db.redis'


local th = coroutine.create(function()
	local res,err = pcall(function()
		local rds = redis.new()
		assert(rds:connect('127.0.0.1',6379))

		assert(rds:set('test.a.1',100))
		assert(rds:incr('test.a.1'))
		print(rds:get('test.a.1'))
	end)
	if not res then
		print('failed exec thread',err)
		error(err)
	end
end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

llae.set_handler()
llae.run()

print('Terminating client')

llae.dump()