package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'net.http'
local log = require 'llae.log'
local fs = require 'llae.fs'
local json = require 'json'
local uv = require 'uv'

local function do_request( url )
	local req = http.createRequest{
		method = 'GET',
		url = url,
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
	print('total body size:', #resp:read_body())
	resp:close()
	print('finished request',url)
end

local th = coroutine.create(function()
	local info = assert(uv.getaddrinfo('google.com'))

	print(json.encode(info))

	do_request('https://github.com/andryblack/llae/blob/master/examples/http_client.lua')
	do_request('http://github.com/zeux/pugixml/releases/download/v1.11/pugixml-1.11.tar.gz')

end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

