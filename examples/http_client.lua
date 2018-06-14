package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'llae.http'
local json = require 'llae.json'


local th = coroutine.create(function()
	local info = assert(llae.getaddrinfo('sandboxgames.ru'))

	print(json.encode(info))

	local req = http.createRequest{
		method = 'GET',
		url = 'https://sandboxgames.ru/',
	}

	local resp = assert(req:exec())
	if resp:get_code() ~= 200 then
		error(resp:get_message())
	end
	print(resp:read_body())

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