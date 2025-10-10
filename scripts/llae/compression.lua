local archive = require 'archive'

---The compression module provides a high-level interface for data compression and decompression using zlib.
---It wraps the lower-level archive module functionality into easy-to-use functions.
---@class llae.compression : archive
local compression = setmetatable({}, {__index=archive})

--- Decompresses data that was compressed using zlib deflate algorithm.
---@param data string|llae.buffer_base The compressed data to inflate
---@return string? The decompressed data on success
---@return string? Error message if decompression fails
function compression.inflate( data )
	local c,err = archive.new_inflate_read()
	if not c then
		return nil,err
	end
	c:finish(data)
	local d = {}
	while true do
		local ch,err = c:read()
		if not ch then
			if err then
				return nil,err
			end
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d,'')
end

--- Compresses the input data using zlib deflate algorithm.
---@param data string|llae.buffer_base The input data to compress
---@return string? The compressed data on success
---@return string? Error message if compression fails
function compression.deflate( data )
	local c,err = archive.new_deflate_read()
	if not c then
		return nil,err
	end
	c:finish(data)
	local d = {}
	while true do
		local ch,err = c:read()
		if not ch then
			if err then
				return nil,err
			end
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d,'')
end

return compression 