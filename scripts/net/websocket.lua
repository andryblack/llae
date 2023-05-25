

local websocket = {
	--server = require 'net.websocket.server',
	client = require 'net.websocket.client'
}




function websocket.createClient( args )
	return websocket.client.new(args)
end


function websocket.createServer( args )
	return websocket.server.new( args )
end

return websocket