local uv = require 'uv'

local fs = setmetatable({},{__index=uv.fs})

function fs.home(  )
	return '${HOME}'
end


return fs