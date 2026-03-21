package.path = package.path .. ';scripts/?.lua'

local async = require 'llae.async'
local log = require 'llae.log'

local uv = require 'uv'

local socket = '/tmp/llae.socket'

local server = uv.pipe_server.new()

async.run(function()

	

	assert(server:bind(socket))

	async.run(function()
		while true do
			log.info('listen')
			local res,err = server:listen(10)
			if not res then
				log.error('listen',err)
				server:stop()
				break
			end
			log.info('accept')
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
		end
	end)

	async.run(function()
		local client = uv.pipe.new()
		log.info('connect')
		assert(client:connect(socket))
		log.info('connected')
		for i=1,10 do
			client:write('msg'..i)
			log.info('client:',client:read())
		end
		client:shutdown()
		server:stop()
	end)
end)

