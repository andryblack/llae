package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'net.http'
local log = require 'llae.log'
local json = require 'json'
local uv = require 'uv'

local th = coroutine.create(function()
	local info = assert(uv.getaddrinfo('google.com'))

	print(json.encode(info))

	local req = http.createRequest{
		method = 'GET',
		url = 'https://github.com/andryblack/llae/blob/master/examples/http_client.lua',
		headers = {
			['Accept'] = '*/*'
		}
	}

	local resp = assert(req:exec())
	if resp:get_code() ~= 200 then
		error(resp:get_message())
	end
	for n,v in resp:foreach_header() do
		log.info(n,':',v)
	end
	print(resp:read_body())
	resp:close()
	print('finished request')

end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

