package.path = package.path .. ';scripts/?.lua'

local async = require 'llae.async'
local log = require 'llae.log'

local uv = require 'uv'

local socket = '/tmp/llae.socket'

local server = uv.pipe_server.new()

async.run(function()

	

	assert(server:bind(socket))

	assert(server:listen(10,function(err)
		if err then
			log.error('listen',err)
			server:stop()
		end
		local client = uv.pipe.new()
		assert(server:accept(client))
		async.run(function()
			while true do
				local data,err = client:read()
				if not data then
					if err then
						log.error('server:',err)
					end
					break
				end
				log.info('server:',data)
				client:write(data)
			end
			client:shutdown()
		end)
	end))

	async.run(function()
		local client = uv.pipe.new()
		assert(client:connect(socket))
		for i=1,10 do
			client:write('msg'..i)
			log.info('client:',client:read())
		end
		client:shutdown()
		server:stop()
	end)
end)

