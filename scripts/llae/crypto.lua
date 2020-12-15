local native = require 'crypto'

local crypto = native


function crypto.md5(  )
	return assert(crypto.md.new('MD5'))
end


return crypto