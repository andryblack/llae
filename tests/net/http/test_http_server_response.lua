local lu = require 'luaunit'
local class = require 'llae.class'

local TestResponse = class(require 'net.http.server_response')

local MockClient = class()

function MockClient:_init()
	self._data = {}
end

function MockClient:write(data)
	if type(data) == 'table' then
		for _,v in ipairs(data) do
			table.insert(self._data,v)
		end
	else
		table.insert(self._data,data)
	end
	return true
end

function MockClient:dump()
	return table.concat( self._data, '' )
end

local MockRequest = class(require 'net.http.headers')

function MockRequest:_init()
	MockRequest.baseclass._init(self)
	self._protocol = 'HTTP'
end

function MockRequest:get_protocol()
	return self._protocol
end

-- Test cases
TestHttpServerResponse = {}

function TestHttpServerResponse:setUp()
	self._client = MockClient.new()
	self._request = MockRequest.new()
 	self._response = TestResponse.new(self._client,self._request)
end

function TestHttpServerResponse:tearDown()

end

function TestHttpServerResponse:check_write(data)
	lu.assertEquals(self._client:dump():gsub('\r\n','\n'),data:gsub('\r\n','\n'))
	lu.assertTrue(self._response:is_finished())
end

function TestHttpServerResponse:test_simple()
	self._response:finish('Data')
	self:check_write([[
HTTP/1.0 200 OK
Content-Length: 4
Content-Type: text/plain
Connection: close

Data]])
end

function TestHttpServerResponse:test_status()
	self._response:status(505,'Wtf')
	self._response:finish('Data')
	self:check_write([[
HTTP/1.0 505 Wtf
Content-Length: 4
Content-Type: text/plain
Connection: close

Data]])
end

function TestHttpServerResponse:test_write()
	self._response:status(505,'Wtf')
	self._response:write('123456')
	self._response:write('7890')
	self._response:finish('Data')
	self:check_write([[
HTTP/1.0 505 Wtf
Content-Length: 14
Content-Type: text/plain
Connection: close

1234567890Data]])
end