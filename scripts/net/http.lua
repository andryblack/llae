local llae = require 'llae'



local http = {
	server = require 'net.http.server',
	request = require 'net.http.request'
}


local function load_platform_certs(ctx)
	local log = require 'llae.log'
	local ssl = require 'ssl'
	local fs = require 'llae.fs'
	if ssl.ctx.default_cafile then
		log.debug('load cert from',ssl.ctx.default_cafile)
		local cert = assert(fs.load_file(ssl.ctx.default_cafile))
		assert(ctx:load_cert(cert))
	else
		log.debug('load system certificates')
		local res,err = ctx:load_system_certs()
		if not res then
			log.error('failed load system cert:',err)
		else
			log.debug('system certificates loaded')
		end
	end
end

--- Creates a new SSL context for HTTP requests.
---@param load_default_cert boolean Load the platform CA bundle
---@return ssl.ctx
function http.create_ssl_ctx(load_default_cert)
	local ssl = require 'ssl'
	local ctx = ssl.ctx.new()
	assert(ctx:init('llae-ssl-seed'))
	if load_default_cert then
		load_platform_certs(ctx)
	end
	return ctx
end

function http.get_ssl_ctx()

	if not http._ssl_ctx then
		http._ssl_ctx = http.create_ssl_ctx(true)
	end
	return http._ssl_ctx
end

---@return net.http.request
function http.createRequest( args )
	return http.request.new(args)
end

---@return net.http.server
function http.createServer( cb )
	return http.server.new(cb)
end

return http