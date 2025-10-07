---@meta llae

---@class llae
local llae = {}

---@param code number?
function llae.stop(code) end
---@param func fun()
function llae.at_exit(func) end
function llae.cancel_sigint() end
---@param obj any
function llae.release_object(obj) end
---@param cont any
---@param ... any
function llae.resume(cont, ...) end
---@return string
function llae.get_host_platform() end

---@class llae.buffer_base
local buffer_base = {}

function buffer_base:__concat() end
function buffer_base:__tostring() end
function buffer_base:__eq() end
function buffer_base:get_len() end
function buffer_base:sub() end
function buffer_base:find() end
function buffer_base:byte() end
function buffer_base:reverse() end
function buffer_base.hex_decode() end
---@param data any
function buffer_base.hex_encode(data) end
function buffer_base.base64_decode() end
function buffer_base.base64_encode() end

---@class llae.buffer : llae.buffer_base
local buffer = {}

---@return integer
function buffer:__len() end

function buffer:__concat() end

function buffer:__eq() end

---@return llae.buffer
function buffer.new() end

---@return llae.buffer
function buffer.alloc() end

---@return integer
function buffer:get_allocated() end

function buffer:self_reverse() end

llae.buffer = buffer

return llae