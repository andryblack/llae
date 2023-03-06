
package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'net.http'
local template = require 'llae.template'
local async = require 'llae.async'

async.run(function()

	local t = template.load('examples/index.thtml')


	assert(http.createServer(function (req, res)
	  
	  res:set_header("Content-Type", "text/html")
	  res:finish(t{
	  	title = 'Hello template'
	  })
	end):listen(1337, '127.0.0.1'))

	print('Server running at http://127.0.0.1:1337/')

end)
