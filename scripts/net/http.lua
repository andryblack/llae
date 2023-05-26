local llae = require 'llae'



local http = {
	server = require 'net.http.server',
	request = require 'net.http.request'
}


function http.get_ssl_ctx()

	if not http._ssl_ctx then
		local log = require 'llae.log'
		local ssl = require 'ssl'
		local fs = require 'llae.fs'
		http._ssl_ctx = ssl.ctx.new()
		assert(http._ssl_ctx:init())
		log.debug('load cert from',ssl.ctx.default_cafile)
		local cert = assert(fs.load_file(ssl.ctx.default_cafile))
		assert(http._ssl_ctx:load_cert(cert))
	end
	return http._ssl_ctx
end


function http.createRequest( args )
	return http.request.new(args)
end


function http.createServer( cb )
	return http.server.new(cb)
end

return http