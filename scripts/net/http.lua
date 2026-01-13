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
		assert(http._ssl_ctx:init('llae-ssl-seed'))
		if ssl.ctx.default_cafile then
			log.debug('load cert from',ssl.ctx.default_cafile)
			local cert = assert(fs.load_file(ssl.ctx.default_cafile))
			assert(http._ssl_ctx:load_cert(cert))
		else
			log.debug('load system certificates')
			local res,err = http._ssl_ctx:load_system_certs()
			if not res then
				log.error('failed load system cert:',err)
			else
				log.debug('system certificates loaded')
			end
		end
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