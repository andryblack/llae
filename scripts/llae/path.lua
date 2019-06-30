local llae = require 'llae'
local file = require 'llae.file'

local path = {}

function path.join( ... )
	return table.concat( table.pack(...) , '/' )
end

function path.mkdir( dir )
	file.mkdir(dir)
end

return path