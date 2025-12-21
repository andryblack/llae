---@meta uv

---@class uv
local uv = {}

---@return string?
---@return string?
uv.exepath = function() end

---@class uv.getaddrinfo.item
---@field family string
---@field socktype string
---@field addr string

---@param host string
---@return uv.getaddrinfo.item[]?
---@return string?
uv.getaddrinfo = function(host) end
---@return string?
---@return string?
uv.cwd = function() end
---@param dir string
---@return boolean?
---@return string?
uv.chdir = function(dir) end
---@param pause integer
uv.pause = function(pause) end
---@param thread thread
---@param delay integer?
uv.resume_delayed = function(thread, delay) end
---@return number
---@return integer
uv.gettimeofday = function() end
---@class uv.interface_address
---@field name string
---@field internal boolean
---@field family integer
---@field address string
---@field netmask string

---@return uv.interface_address[]?
---@return string?
uv.interface_addresses = function() end
---@param title string
---@return boolean?
---@return string?
uv.set_process_title = function(title) end
---@return integer
uv.get_free_memory = function() end
---@return integer
uv.get_total_memory = function() end
---@return integer
uv.get_constrained_memory = function() end
---@return integer
uv.hrtime = function() end
---@param ms integer
uv.sleep = function(ms) end
---@param size integer
---@return string?
---@return string?
uv.random = function(size) end
---@param active boolean?
uv.print_handles = function(active) end
---@type number
uv.AF_INET = 0
---@type number
uv.AF_INET6 = 0

---@class uv.handle
local handle = {}

---@class uv.stream : uv.handle
local stream = {}
---@return llae.buffer?
---@return string?
function stream:read() end
---@return boolean?
---@return string?
function stream:write(...) end
---@param file uv.file
---@return boolean?
---@return string?
function stream:send(file) end
---@return boolean?
---@return string?
function stream:shutdown() end
function stream:close() end
function stream:stop_read() end
---@param buffer any
function stream:add_read_buffer(buffer) end

---@class uv.server : uv.handle
local server = {}
---@param backlog integer
---@param func function
---@return boolean?
---@return string?
function server:listen(backlog,func) end
---@param client uv.stream
---@return boolean?
---@return string?
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
---@return boolean?
---@return string?
function tcp_connection:connect(host,port) end
---@return string?
---@return integer|string?
function tcp_connection:getppername() end
---@param enable boolean
---@param delay integer?
---@return integer?
---@return string?
function tcp_connection:keepalive(enable, delay) end
---@param enable boolean
---@return integer?
---@return string?
function tcp_connection:nodelay(enable) end
uv.tcp_connection = tcp_connection


---@class uv.udp : uv.handle
local udp = {}
---@return uv.udp
function udp.new() end
---@param host string?
---@param port integer?
---@param flags integer?
---@return boolean?
---@return string?
function udp:bind(host,port,flags) end
---@param data any
---@param host string?
---@param port integer?
---@return boolean?
---@return string?
function udp:send(data,host,port) end
---@param data any
---@param host string?
---@param port integer?
---@return integer?
---@return string?
function udp:try_send(data, host, port) end
---@return llae.buffer?
---@return string?
function udp:recv() end
---@param host string
---@param port integer
---@return boolean?
---@return string?
function udp:connect(host, port) end
function udp:disconnect() end
function udp:stop_recv() end
---@param buffer llae.buffer
function udp:add_buffer(buffer) end
function udp:close() end
---@param ttl integer
---@return integer?
---@return string?
function udp:set_ttl(ttl) end
---@param enable boolean
---@return integer?
---@return string?
function udp:set_broadcast(enable) end
---@param multicast_addr string
---@param interface_addr string
---@param membership integer
---@return integer?
---@return string?
function udp:set_membershift(multicast_addr, interface_addr, membership) end
---@param multicast_addr string
---@param interface_addr string
---@param source_addr string
---@param membership integer
---@return integer?
---@return string?
function udp:set_source_membership(multicast_addr, interface_addr, source_addr, membership) end
---@param enable boolean
---@return integer?
---@return string?
function udp:set_multicast_loop(enable) end
---@param ttl integer
---@return integer?
---@return string?
function udp:set_multicast_ttl(ttl) end
---@param interface_addr string
---@return integer?
---@return string?
function udp:set_multicast_interface(interface_addr) end
---@return string?
---@return integer|string?
function udp:getpeername() end
---@return string?
---@return integer|string?
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
---@param mode integer
---@return integer?
---@return string?
function tty:set_mode(mode) end
---@return integer?
---@return string?
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
---@param events integer
---@return boolean?
---@return string?
function poll:poll(events) end
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
---@param signal integer
---@return integer?
---@return string?
function process:kill(signal) end
---@return integer?
---@return string?
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
---@return boolean?
---@return string?
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
---@class uv.os.uname
---@field sysname string
---@field release string
---@field version string
---@field machine string

---@return string?
---@return string?
function os.homedir() end
---@return string?
---@return string?
function os.tmpdir() end
---@param name string
---@return string?
---@return string?
function os.getenv(name) end
---@param name string
---@param value string
---@return boolean?
---@return string?
function os.setenv(name, value) end
---@return table<string,string>?
---@return string?
function os.getallenv() end
---@param name string
---@return boolean?
---@return string?
function os.unsetenv(name) end
---@return string?
---@return string?
function os.gethostname() end
---@return uv.os.uname?
---@return string?
function os.uname() end
---@return integer
function os.getpid() end
---@param pid integer?
---@return integer?
---@return string?
function os.getpriority(pid) end
---@param pid integer?
---@param priority integer
---@return boolean?
---@return string?
function os.setpriority(pid, priority) end

uv.os = os

---@class uv.fs
local fs = {}

---@param path string
---@param mode integer?
---@return boolean?
---@return string?
function fs.mkdir(path, mode) end
---@param path string
---@return boolean?
---@return string?
function fs.rmdir(path) end
---@param filename string
---@return true?
---@return string?
function fs.unlink(filename) end
---@param src string
---@param dst string
---@param flags integer?
---@return boolean?
---@return string?
function fs.copyfile(src, dst, flags) end
---@param src string
---@param dst string
---@return boolean?
---@return string?
function fs.rename(src, dst) end

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
---@param path string
---@param mode integer
---@return boolean?
---@return string?
function fs.chmod(path, mode) end

fs.O_RDONLY = 0
fs.O_RDWR = 0
fs.O_WRONLY = 0
fs.O_CREAT = 0
fs.O_APPEND = 0

---@class uv.file
local file = {}
---@return boolean?
---@return string?
function file:close() end
---@return boolean?
---@return string?
function file:write(...) end
---@param size integer?
---@return llae.buffer?
---@return string?
function file:read(size) end
---@param offset integer
---@param whence integer?
---@return integer?
---@return string?
function file:seek(offset, whence) end
---@return integer?
---@return string?
function file:tell() end
---@return integer?
---@return string?
function file:get_handle() end
fs.file = file

uv.fs = fs

return uv