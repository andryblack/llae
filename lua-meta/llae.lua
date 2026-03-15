---@meta llae

---@class llae
local llae = {}

---@param code integer?
function llae.stop(code) end
---@param func function
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

---@param other llae.buffer_base|string
---@return string
function buffer_base:__concat(other) end
---@return string
function buffer_base:__tostring() end
---@param other llae.buffer_base|string
---@return boolean
function buffer_base:__eq(other) end
---@return integer
function buffer_base:get_len() end
---@param start integer
---@param finish integer?
---@return string
function buffer_base:sub(start, finish) end
---@param pattern string
---@param init integer?
---@return integer?
function buffer_base:find(pattern, init) end
---@param start integer?
---@param finish integer?
---@return ...integer
function buffer_base:byte(start, finish) end
---@return llae.buffer_base
function buffer_base:reverse() end
---@param data string
---@return llae.buffer?
---@return string?
function buffer_base.hex_decode(data) end
---@param data string|llae.buffer_base
---@return llae.buffer
function buffer_base.hex_encode(data) end
---@param data string|llae.buffer_base
---@return llae.buffer?
---@return string?
function buffer_base.base64_decode(data) end
---@param data string|llae.buffer_base
---@return llae.buffer
function buffer_base.base64_encode(data) end
---@param a string|llae.buffer_base
---@param b string|llae.buffer_base
---@return llae.buffer
function buffer_base.xor(a,b) end
---@param other string|llae.buffer_base
---@return llae.buffer
function buffer_base:__concat(other) end

---@class llae.buffer : llae.buffer_base
local buffer = {}

---@return integer
function buffer:__len() end
---@param other llae.buffer|string
---@return string
function buffer:__concat(other) end
---@param other llae.buffer|string
---@return boolean
function buffer:__eq(other) end
---@param data string?
---@return llae.buffer
function buffer.new(data) end
---@param size integer
---@return llae.buffer
function buffer.alloc(size) end
---@return integer
function buffer:get_allocated() end

function buffer:self_reverse() end

llae.buffer = buffer

---@class llae.writable_buffer : llae.buffer_base
local writable_buffer = {}

---@param size integer
---@return llae.writable_buffer
function writable_buffer.alloc(size) end
---@param data string
---@return llae.writable_buffer
function writable_buffer.new(data) end

---@param offset integer
---@param data string|llae.buffer_base
---@return boolean
function writable_buffer:write(offset, data) end

llae.writable_buffer = writable_buffer

---@class llae.native.log_handler
local log_handler = {}
function log_handler:print(...) end

---@class llae.native.log
local log = {}
log.level = {
	debug = 0,
    info = 1,
    warning = 2,
    error = 3,
    fatal = 4,
}
log.print_level = {
    debug = {},
    info = {},
    warning = {},
    error = {},
    fatal = {},
}
function log.print(...) end
---@param level integer
---@param message string
function log.write(level, message) end
---@return llae.native.log_handler
function log.add_stdout_handler() end
function log.remove_stdout_handler() end
---@param file uv.file
---@param with_time boolean
---@return llae.native.log_handler
function log.add_file_handler(file, with_time) end
---@param level integer
---@param prefix string
function log.set_console_prefix(level, prefix) end
---@param handler llae.native.log_handler
function log.add_handler(handler) end
---@param handler llae.native.log_handler
function log.remove_handler(handler) end

llae.log = log

---@class llae.error
local error = {}

---@generic T
---@class llae.promise<T>
local promise = {}

---@return T? promise result
---@return llae.error? promise error
function promise:await() end

llae.promise = promise

return llae