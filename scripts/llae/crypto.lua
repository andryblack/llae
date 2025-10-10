local native = require 'crypto'

---The LLAE crypto module provides cryptographic functionality through a combination of Lua and native C++ implementations using the mbedTLS library.
---@class llae.crypto : crypto
local crypto = setmetatable({}, {__index=native})

--- Creates a new MD5 message digest instance.
---@return crypto.md A new MD5 digest instance
function crypto.md5(  )
	return assert(crypto.md.new('MD5'))
end

--- Calculates MD5 hash of the input data.
---@param md crypto.md The message digest instance
---@param data string|llae.buffer_base The input data to hash
---@return llae.buffer? The hash result on success
---@return string? Error message if hashing fails
local function calc_md(md,data)
	local res,err = md:update(data)
	if not res then
		return nil, err
	end
	return md:finish()
end

--- Calculates MD5 hash of the input data.
---@param data string|llae.buffer_base The input data to hash
---@return llae.buffer? The MD5 hash result on success
---@return string? Error message if hashing fails
function crypto.md5sum(data)
	return calc_md(crypto.md5(),data)
end

--- Creates a new SHA256 message digest instance.
---@return crypto.md A new SHA256 digest instance
function crypto.sha256()
	return assert(crypto.md.new('SHA256'))
end

--- Calculates SHA256 hash of the input data.
---@param data string|llae.buffer_base The input data to hash
---@return llae.buffer? The SHA256 hash result on success
---@return string? Error message if hashing fails
function crypto.sha256sum(data)
	return calc_md(crypto.sha256(),data)
end

return crypto