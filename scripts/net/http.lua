local llae = require 'llae'



local http = {
	server = require 'net.http.server',
	request = require 'net.http.request'
}




function http.createRequest( args )
	return http.request.new(args)
end


function http.createServer( cb , config )
	return http.server.new(cb, config )
end

return http