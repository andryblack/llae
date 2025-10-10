---@meta archive

---@class archive
local archive = {}

---@return integer
function archive.get_allocated() end

---@param level integer?
---@return archive.zcompress_deflate_read?
---@return string?
function archive.new_deflate_read(level) end

---@param options table?
---@return archive.zcompress_gzip_read?
---@return string?
function archive.new_gzip_read(options) end

---@param stream uv.stream
---@return archive.zcompress_to_stream?
---@return string?
function archive.new_deflate_to_stream(stream) end

---@param try_raw boolean?
---@return archive.zuncompress_inflate_read?
---@return string?
function archive.new_inflate_read(try_raw) end

---@return archive.zuncompress_gzip_read?
---@return string?
function archive.new_gunzip_read() end

---@param stream uv.stream
---@return archive.zuncompress_to_stream?
---@return string?
function archive.new_inflate_to_stream(stream) end

---@class archive.zcompress
local zcompress = {}

---@param data string|llae.buffer_base
---@return boolean?
---@return string?
function zcompress:write(data) end

---@param data any?
---@return boolean?
---@return string?
function zcompress:finish(data) end

---@param file uv.file
---@return boolean?
---@return string?
function zcompress:send(file) end

---@class archive.zcompress_deflate_read : archive.zcompress
local zcompress_deflate_read = {}

---@return llae.buffer?
---@return string?
function zcompress_deflate_read:read() end

---@param buffer llae.buffer
---@return integer?
---@return string?
function zcompress_deflate_read:read_buffer(buffer) end

---@class archive.zcompress_gzip_read : archive.zcompress_deflate_read
local zcompress_gzip_read = {}

---@class archive.zcompress_to_stream : archive.zcompress
local zcompress_to_stream = {}

---@class archive.zuncompress
local zuncompress = {}

---@param data string|llae.buffer_base
---@return boolean?
---@return string?
function zuncompress:write(data) end

---@param data any?
---@return boolean?
---@return string?
function zuncompress:finish(data) end

---@param file uv.file
---@return boolean?
---@return string?
function zuncompress:send(file) end

---@class archive.zuncompress_inflate_read : archive.zuncompress
local zuncompress_inflate_read = {}

---@return llae.buffer?
---@return string?
function zuncompress_inflate_read:read() end

---@param buffer llae.buffer
---@return integer?
---@return string?
function zuncompress_inflate_read:read_buffer(buffer) end

---@class archive.zuncompress_gzip_read : archive.zuncompress_inflate_read
local zuncompress_gzip_read = {}

---@class archive.zuncompress_to_stream : archive.zuncompress
local zuncompress_to_stream = {}

archive.zcompress = zcompress
archive.zcompress_deflate_read = zcompress_deflate_read
archive.zcompress_gzip_read = zcompress_gzip_read
archive.zcompress_to_stream = zcompress_to_stream
archive.zuncompress = zuncompress
archive.zuncompress_inflate_read = zuncompress_inflate_read
archive.zuncompress_gzip_read = zuncompress_gzip_read
archive.zuncompress_to_stream = zuncompress_to_stream

return archive
