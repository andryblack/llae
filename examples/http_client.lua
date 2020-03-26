package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'llae.http'
local json = require 'json'
local uv = require 'uv'

local th = coroutine.create(function()
	local info = assert(uv.getaddrinfo('google.com'))

	print(json.encode(info))

	local req = http.createRequest{
		method = 'GET',
		url = 'https://google.com/',
	}

	local resp = assert(req:exec())
	if resp:get_code() ~= 200 then
		resp:close()
		error(resp:get_message())
	end
	print(resp:read_body())
	print('finished request')

end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

