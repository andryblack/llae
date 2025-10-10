---@meta posix

---The posix module provides POSIX system call functionality.
---Supports file operations, file descriptors, and system-level operations.
---@class posix
local posix = {}

--- Opens a file and returns a file descriptor.
---@param pathname string The file path to open
---@param flags integer File access flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
---@param mode integer? Optional file mode for creation (S_IRWXU, S_IRUSR, etc.)
---@return posix.fd? The file descriptor on success
---@return string? Error message if opening fails
function posix.open(pathname, flags, mode) end

--- Duplicates a file descriptor to another file descriptor.
---@param fd posix.fd The source file descriptor
---@param from integer The target file descriptor number
---@return integer? The duplicated file descriptor on success
---@return string? Error message if duplication fails
function posix.dup2(fd, from) end

---File descriptor for POSIX file operations.
---@class posix.fd
local fd = {}

--- Closes the file descriptor.
---@return boolean? True on success
---@return string? Error message if close fails
function fd:close() end

--- Reads data from the file descriptor.
---@param buffer_or_size llae.buffer|integer Buffer to read into or size to allocate
---@return llae.buffer? The read data on success
---@return string? Error message if read fails
function fd:read(buffer_or_size) end

--- Writes data to the file descriptor.
---@param data string|llae.buffer_base The data to write
---@return integer? Number of bytes written on success
---@return string? Error message if write fails
function fd:write(data) end

--- Performs file control operations on the file descriptor.
---@param cmd integer File control command (F_GETFD, F_SETFD, F_GETFL, F_SETFL, etc.)
---@param arg integer? Optional argument for set commands
---@return integer? Command result on success
---@return string? Error message if fcntl fails
function fd:fcntl(cmd, arg) end

posix.fd = fd

-- File access flags
---@type integer
posix.O_RDONLY = 0     -- Open for reading only
---@type integer
posix.O_WRONLY = 0     -- Open for writing only
---@type integer
posix.O_RDWR = 0       -- Open for reading and writing
---@type integer
posix.O_CREAT = 0      -- Create file if it doesn't exist
---@type integer
posix.O_EXCL = 0       -- Fail if file exists (with O_CREAT)
---@type integer
posix.O_APPEND = 0     -- Append to file
---@type integer
posix.O_TRUNC = 0      -- Truncate file to zero length
---@type integer
posix.O_NOCTTY = 0     -- Don't assign controlling terminal
---@type integer
posix.O_ASYNC = 0      -- Enable signal-driven I/O
---@type integer
posix.O_SYNC = 0       -- Synchronous I/O
---@type integer
posix.O_DIRECT = 0     -- Direct I/O
---@type integer
posix.O_NONBLOCK = 0   -- Non-blocking I/O

-- File permission flags
---@type integer
posix.S_IRWXU = 0      -- Read, write, execute by owner
---@type integer
posix.S_IRUSR = 0      -- Read permission for owner
---@type integer
posix.S_IWUSR = 0      -- Write permission for owner
---@type integer
posix.S_IXUSR = 0      -- Execute permission for owner
---@type integer
posix.S_IRWXG = 0      -- Read, write, execute by group
---@type integer
posix.S_IRGRP = 0      -- Read permission for group
---@type integer
posix.S_IWGRP = 0      -- Write permission for group
---@type integer
posix.S_IXGRP = 0      -- Execute permission for group
---@type integer
posix.S_IRWXO = 0      -- Read, write, execute by others
---@type integer
posix.S_IROTH = 0      -- Read permission for others
---@type integer
posix.S_IWOTH = 0      -- Write permission for others
---@type integer
posix.S_IXOTH = 0      -- Execute permission for others

-- File control commands
---@type integer
posix.F_GETFD = 0      -- Get file descriptor flags
---@type integer
posix.F_SETFD = 0      -- Set file descriptor flags
---@type integer
posix.F_GETFL = 0      -- Get file status flags
---@type integer
posix.F_SETFL = 0      -- Set file status flags
---@type integer
posix.F_GETOWN = 0     -- Get process or process group ID
---@type integer
posix.F_SETOWN = 0     -- Set process or process group ID
---@type integer
posix.F_GETSIG = 0     -- Get signal sent to owner
---@type integer
posix.F_SETSIG = 0     -- Set signal sent to owner
---@type integer
posix.F_GETLEASE = 0   -- Get lease type
---@type integer
posix.F_SETLEASE = 0   -- Set lease type
---@type integer
posix.F_NOTIFY = 0     -- Request notification

-- Standard file descriptors
---@type integer
posix.STDIN_FILENO = 0  -- Standard input file descriptor
---@type integer
posix.STDOUT_FILENO = 0 -- Standard output file descriptor
---@type integer
posix.STDERR_FILENO = 0 -- Standard error file descriptor

return posix