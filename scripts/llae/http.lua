local llae = require 'llae'



local http = {
	server = require 'llae.http.server',
	request = require 'llae.http.request'
}




function http.createRequest( args )
	return http.request.new(args)
end


function http.createServer( cb )
	return http.server.new(cb)
end

return http