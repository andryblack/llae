local native = require 'crypto'

local crypto = native


function crypto.md5(  )
	return assert(crypto.md.new('MD5'))
end

local function calc_md(md,data)
	local res,err = md:update(data)
	if not res then
		return nil, err
	end
	return md:finish()
end

function crypto.md5sum(data)
	return calc_md(crypto.md5(),data)
end

function crypto.sha256()
	return assert(crypto.md.new('SHA256'))
end

function crypto.sha256sum(data)
	return calc_md(crypto.sha256(),data)
end

return crypto