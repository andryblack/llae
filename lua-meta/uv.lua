---@meta uv

---@class uv
local uv = {}

---@return string
uv.exepath = function() end

---@class uv.getaddrinfo.item
---@field family string
---@field socktype string
---@field addr string

---@param host string
---@return uv.getaddrinfo.item[]?
---@return string?
uv.getaddrinfo = function(host) end
---@return string
uv.cwd = function() end
---@param dir string
---@return boolean
uv.chdir = function(dir) end
---@param pause boolean
---@return boolean
uv.pause = function(pause) end
---@return number
uv.gettimeofday = function() end
---@return table
uv.interface_addresses = function() end
---@param title string
---@return boolean
uv.set_process_title = function(title) end
---@return number
uv.get_free_memory = function() end
---@return number
uv.get_total_memory = function() end
---@return number
uv.get_constrained_memory = function() end
---@return number
uv.hrtime = function() end
---@param ms number
---@return boolean
uv.sleep = function(ms) end
---@return number
uv.random = function(s) end
---@return boolean
uv.print_handles = function() end
---@type number
uv.AF_INET = 0
---@type number
uv.AF_INET6 = 0

---@class uv.handle
local handle = {}

---@class uv.stream : uv.handle
local stream = {}
function stream:read() end
function stream:write(...) end
function stream:send() end
function stream:shutdown() end
function stream:close() end
function stream:stop_read() end
---@param buffer any
function stream:add_read_buffer(buffer) end

---@class uv.server : uv.handle
local server = {}
---@param backlog integer
---@param func function
function server:listen(backlog,func) end
---@param client uv.stream
function server:accept(client) end
function server:stop() end

---@class uv.tcp_server : uv.server
local tcp_server = {}
---@return uv.tcp_server
function tcp_server.new() end
---@param addr string
---@param port integer
function tcp_server:bind(addr,port) end
uv.tcp_server = tcp_server


---@class uv.tcp_connection : uv.stream
local tcp_connection = {}
---@return uv.tcp_connection
function tcp_connection.new() end
---@param host string
---@param port integer
---@return boolean?,string?
function tcp_connection:connect(host,port) end
function tcp_connection:getppername() end
function tcp_connection:keepalive() end
function tcp_connection:nodelay() end
uv.tcp_connection = tcp_connection


---@class uv.udp : uv.handle
local udp = {}
---@return uv.udp
function udp.new() end
---@param host string?
---@param port integer?
---@param flags integer?
function udp:bind(host,port,flags) end
---@param data any
---@param host string?
---@param port integer?
function udp:send(data,host,port) end
function udp:try_send() end
function udp:recv() end
function udp:connect() end
function udp:disconnect() end
function udp:stop_recv() end
function udp:add_buffer() end
function udp:close() end
function udp:set_ttl() end
function udp:set_broadcast() end
function udp:set_membershift() end
function udp:set_source_membership() end
function udp:set_multicast_loop() end
function udp:set_multicast_ttl() end
function udp:set_multicast_interface() end
function udp:getpeername() end
function udp:getsockname() end

udp.IPV6ONLY = 0
udp.REUSEADDR = 0
udp.PARTIAL = 0
udp.LEAVE_GROUP = 0
udp.JOIN_GROUP = 0

uv.udp = udp


---@class uv.tty : uv.stream
local tty = {}

---@return uv.tty
function tty.new() end
function tty:set_mode() end
function tty:reset_mode() end

tty.MODE_NORMAL = 0
tty.MODE_RAW = 0
tty.MODE_IO = 0

---@param fd posix.fd|integer
---@return uv.tty
function tty.new(fd) end

uv.tty = tty


---@class uv.poll : uv.handle
local poll = {}
---@return uv.poll
function poll.new() end
function poll:stop() end
function poll:poll() end
poll.READABLE = 0
poll.WRITABLE = 0
poll.PRIORITIZED = 0
poll.DISCONNECT = 0
uv.poll = poll


---@class uv.process
local process = {}
---@class uv.process.spawn_args
---@field file string
---@field flags integer?
---@field args string[]?
---@field env table<string,string>?
---@field cwd string?
---@field streams [integer,integer|uv.stream?][]?

---@param args uv.process.spawn_args
---@return uv.process?,string?
function process.spawn(args) end
function process:kill() end
function process:wait_exit() end

process.IGNORE = 0
process.CREATE_PIPE = 0
process.INHERIT_FD = 0
process.INHERIT_STREAM = 0
process.READABLE_PIPE = 0
process.WRITABLE_PIPE = 0
process.NONBLOCK_PIPE = 0
process.PROCESS_DETACHED = 0

uv.process = process


---@class uv.pipe : uv.stream
local pipe = {}
---@param fd integer?
---@return uv.pipe
function pipe.new(fd) end
---@param path string
function pipe:connect(path) end
uv.pipe = pipe

---@class uv.pipe_server : uv.server
local pipe_server = {}
---@return uv.pipe_server
function pipe_server.new() end
---@param filename string
function pipe_server:bind(filename) end
uv.pipe_server = pipe_server


---@class uv.timer
local timer = {}

    
---@class uv.timer_lcb : uv.timer
local timer_lcb = {}

---@return uv.timer_lcb
function timer_lcb.new() end
---@param func fun(uv.timer_lcb)
---@param delay integer
---@param rep integer?
---@return integer?,string?
function timer_lcb:start(func,delay,rep) end
---@return integer?,string?
function timer_lcb:stop() end

uv.timer = timer_lcb


---@class uv.timer_wait
local timer_wait = {}

uv.timer_wait = timer_wait


---@class uv.lua_signal : uv.handle
local lua_signal = {}
---@param signal integer
---@param func function
---@return uv.lua_signal
function lua_signal.oneshot(signal,func) end
function lua_signal:stop() end
function lua_signal:unref() end

uv.signal = lua_signal


---@class uv.async_wait
local async_wait = {}

uv.async = async_wait


---@class uv.os
local os = {}
function os.homedir() end
function os.tmpdir() end
function os.getenv() end
function os.setenv() end
function os.getallenv() end
function os.unsetenv() end
function os.gethostname() end
function os.uname() end
function os.getpid() end
function os.getpriority() end
function os.setpriority() end

uv.os = os

---@class uv.fs
local fs = {}

---@param path string
function fs.mkdir(path) end
---@param path string
function fs.rmdir(path) end
---@param filename string
---@return true?
---@return string?
function fs.unlink(filename) end
function fs.copyfile() end
function fs.rename() end

---@class uv.fs.stat.result
---@field isfile true?
---@field isdir true?
---@field size integer
---@field mtim {sec:integer,nsec:integer}

---@param filename string
---@return uv.fs.stat.result?
---@return string?
function fs.stat(filename) end

---@class uv.fs.scandir.entry
---@field name string
---@field isfile true?
---@field isdir true?
---@field islink true?

---@param path string
---@return uv.fs.scandir.entry[]?
---@return string?
function fs.scandir(path) end
---@param filename string
---@param flags integer?
---@return uv.file?
---@return string?
function fs.open(filename,flags) end
function fs.chmod() end

fs.O_RDONLY = 0
fs.O_RDWR = 0
fs.O_WRONLY = 0
fs.O_CREAT = 0
fs.O_APPEND = 0

---@class uv.file
local file = {}
function file:close() end
function file:write(...) end
---@param size integer?
function file:read(size) end
function file:seek() end
function file:tell() end
function file:get_handle() end
fs.file = file

uv.fs = fs

return uv