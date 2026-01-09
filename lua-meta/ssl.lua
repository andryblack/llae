---@meta ssl

---The SSL module provides SSL/TLS functionality through mbedTLS library.
---Supports SSL/TLS connections, certificate management, and secure communication.
---@class ssl
local ssl = {}

---SSL context for managing SSL/TLS configuration and certificates.
---@class ssl.ctx
---@field default_cafile string? Default certificate authority file path for the current platform.
local ctx = {}

--- Creates a new SSL context with optional random number generator.
---@param random crypto.random? Optional random number generator
---@return ssl.ctx The SSL context instance
function ctx.new(random) end

--- Initializes the SSL context with random number generation.
---@param entropy crypto.entropy? Optional entropy source
---@param pers string? Optional persistent string
---@return boolean? True on success
---@return string? Error message if initialization fails
function ctx:init(entropy, pers) end

--- Loads a certificate from buffer data (PEM or DER format).
---@param cert_data llae.buffer_base The certificate data
---@return boolean? True on success
---@return string? Error message if loading fails
function ctx:load_cert(cert_data) end

--- Loads system certificates from the operating system certificate store.
---@return boolean? True on success
---@return string? Error message if loading fails
function ctx:load_system_certs() end

--- Sets the debug threshold for SSL debugging output.
---@param threshold integer Debug threshold level
function ctx.set_debug_threshold(threshold) end

ssl.ctx = ctx

---SSL connection for secure communication over TCP streams.
---@class ssl.connection
local connection = {}

--- Creates a new SSL connection with context and underlying stream.
---@param ctx ssl.ctx The SSL context
---@param stream uv.stream The underlying TCP stream
---@return ssl.connection The SSL connection instance
function connection.new(ctx, stream) end

--- Configures the SSL connection with default settings.
---@return boolean? True on success
---@return string? Error message if configuration fails
function connection:configure() end

--- Sets the hostname for SSL certificate verification.
---@param hostname string The hostname to verify against
---@return boolean? True on success
---@return string? Error message if setting hostname fails
function connection:set_host(hostname) end

--- Performs SSL handshake with the remote peer.
---@return boolean? True on success
---@return string? Error message if handshake fails
function connection:handshake() end

--- Writes data to the SSL connection.
---@param data string|llae.buffer_base The data to write
---@return boolean? True on success
---@return string? Error message if write fails
function connection:write(data) end

--- Reads data from the SSL connection.
---@return llae.buffer? The read data on success
---@return string? Error message if read fails
function connection:read() end

--- Closes the SSL connection.
---@return boolean? True on success
---@return string? Error message if close fails
function connection:close() end

--- Initiates SSL shutdown sequence.
---@return boolean? True on success
---@return string? Error message if shutdown fails
function connection:shutdown() end

--- Starts reading from the SSL connection.
---@return integer? Status code on success
---@return string? Error message if start fails
function connection:start_read() end

--- Stops reading from the SSL connection.
function connection:stop_read() end

--- Adds a read buffer to the SSL connection.
---@param buffer llae.buffer The buffer to add
function connection:add_read_buffer(buffer) end

ssl.connection = connection

return ssl
