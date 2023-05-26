package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local websocket = require 'net.websocket'
local http = require 'net.http'
local log = require 'llae.log'
local fs = require 'llae.fs'
local json = require 'json'
local uv = require 'uv'
local async = require 'llae.async'
local dns = require 'net.dns'


local th = coroutine.create(function()

	local cert = [[
-----BEGIN CERTIFICATE-----
MIIDlTCCAn2gAwIBAgIUOLxr3q7Wd/pto1+2MsW4fdRheCIwDQYJKoZIhvcNAQEL
BQAwWjELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMRQwEgYDVQQHDAtMb3MgQW5n
ZWxlczEOMAwGA1UECgwFQmVhc3QxGDAWBgNVBAMMD3d3dy5leGFtcGxlLmNvbTAe
Fw0yMTA3MDYwMTQ5MjVaFw00ODExMjEwMTQ5MjVaMFoxCzAJBgNVBAYTAlVTMQsw
CQYDVQQIDAJDQTEUMBIGA1UEBwwLTG9zIEFuZ2VsZXMxDjAMBgNVBAoMBUJlYXN0
MRgwFgYDVQQDDA93d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQCz0GwgnxSBhygxBdhTHGx5LDLIJSuIDJ6nMwZFvAjdhLnB/vOT
Lppr5MKxqQHEpYdyDYGD1noBoz4TiIRj5JapChMgx58NLq5QyXkHV/ONT7yi8x05
P41c2F9pBEnUwUxIUG1Cb6AN0cZWF/wSMOZ0w3DoBhnl1sdQfQiS25MTK6x4tATm
Wm9SJc2lsjWptbyIN6hFXLYPXTwnYzCLvv1EK6Ft7tMPc/FcJpd/wYHgl8shDmY7
rV+AiGTxUU35V0AzpJlmvct5aJV/5vSRRLwT9qLZSddE9zy/0rovC5GML6S7BUC4
lIzJ8yxzOzSStBPxvdrOobSSNlRZIlE7gnyNAgMBAAGjUzBRMB0GA1UdDgQWBBR+
dYtY9zmFSw9GYpEXC1iJKHC0/jAfBgNVHSMEGDAWgBR+dYtY9zmFSw9GYpEXC1iJ
KHC0/jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBzKrsiYywl
RKeB2LbddgSf7ahiQMXCZpAjZeJikIoEmx+AmjQk1bam+M7WfpRAMnCKooU+Utp5
TwtijjnJydkZHFR6UH6oCWm8RsUVxruao/B0UFRlD8q+ZxGd4fGTdLg/ztmA+9oC
EmrcQNdz/KIxJj/fRB3j9GM4lkdaIju47V998Z619E/6pt7GWcAySm1faPB0X4fL
FJ6iYR2r/kJLoppPqL0EE49uwyYQ1dKhXS2hk+IIfA9mBn8eAFb/0435A2fXutds
qhvwIOmAObCzcoKkz3sChbk4ToUTqbC0TmFAXI5Upz1wnADzjpbJrpegCA3pmvhT
7356drqnCGY9
-----END CERTIFICATE-----
]]

	assert(http.get_ssl_ctx():load_cert(cert))

	dns.override('www.example.com',{
		socktype = 'tcp',
		addr = '127.0.0.1'
	})

	
	local ws = websocket.createClient{
		url = 'wss://www.example.com:8080',
	}

	local len = 1
	uv.timer.new():start(function (  )
		async.run(function()
			assert(ws:text(string.rep('PING',len)))
			len = len + 1
			if len > 32 then
				len = 1
			end
		end)
	end,1000,10)

	assert(ws:start(function(msg,err)
		if msg then
			log.info('RX:',msg)
		else
			log.error(err)
		end
	end))

	ws:close()
	print('finished request')

end)

local res,err = coroutine.resume(th)
if not res then
	print('failed main thread',err)
	error(err)
end

