local uv = require 'uv'
local path = require 'llae.path'

---The fs module provides a comprehensive set of functions for working with the file system.
---It wraps the libuv filesystem operations and adds additional high-level functionality.
---@class llae.fs : uv.fs
local fs = setmetatable({},{__index=uv.fs})

--- Returns the user's home directory path.
fs.home = uv.os.homedir

--- Gets the current working directory.
fs.pwd = uv.cwd

--- Gets the current working directory (alias for pwd).
fs.cwd = uv.cwd

--- Changes the current working directory.
fs.chdir = uv.chdir

--- Gets the path to the current executable.
fs.exepath = uv.exepath

--- Checks if a path points to a regular file.
---@param fn string Path to check
---@return boolean True if the path points to a regular file, false otherwise (including when the path doesn't exist)
function fs.isfile( fn )
	local st = fs.stat(fn)
	return st and st.isfile or false
end

--- Checks if a path points to a directory.
---@param fn string Path to check
---@return boolean True if the path points to a directory, false otherwise (including when the path doesn't exist)
function fs.isdir( fn )
	local st = fs.stat(fn)
	return st and st.isdir or false
end

--- Recursively removes a directory and all its contents.
---@param dir string Path to the directory to remove
---@return boolean? True if successful
---@return string? Error message if failed
function fs.rmdir_r(dir)
	local files,err = fs.scandir(dir)
	if not files then
		return files,err
	end
	for _,f in ipairs(files) do
		local fn = path.join(dir,f.name)
		if f.isdir then
			local res,err = fs.rmdir_r(fn)
			if not res then
				return res,err
			end
		else
			local res,err = fs.unlink(fn)
			if not res then
				return res,err
			end
		end
	end
	return uv.fs.rmdir(dir)
end

--- Opens a file with enhanced error handling.
---@param filename string Path to the file to open
---@param mode integer File access mode flags
---@return uv.file? File handle on success
---@return string? Error message if opening fails
function fs.open( filename, mode )
	local f,er = uv.fs.open(filename,mode)
	if not f then
		return nil,'failed open ' .. filename .. ' ' .. tostring(er)
	end
	return f,er
end

--- Internal function for recursive file scanning.
---@type fun(res:string[],dir:string,r:string):string[]?,string?
local scanfiles_r
scanfiles_r = function(res,dir,r)
	local f,e = fs.scandir(dir)
	if not f then
		return f,e
	end
	for _,v in ipairs(f) do
		if v.isfile then
			table.insert(res,r..v.name)
		elseif v.isdir then
			local ff,ee = scanfiles_r(res,dir .. '/' .. v.name, r..v.name .. '/')
			if not ff then
				return f,ee
			end
		end
	end
	return res
end

--- Recursively scans a directory and returns a list of all files.
---@param dir string Directory to scan
---@return string[]? Array of file paths relative to the scanned directory on success
---@return string? Error message if operation failed
function fs.scanfiles_r( dir )
	local res = {}
	return scanfiles_r(res,dir,'')
end


--- Creates a directory and all necessary parent directories.
---@param dir string Path to the directory to create
---@return boolean? True if successful
---@return string? Error message if failed
function fs.mkdir_r(dir)
	---@type string[]
	local components = {}
	while dir and dir~='/' and dir~='' and (not fs.isdir(dir)) do
		local f = path.basename(dir)
		table.insert(components,1,f)
		dir = path.dirname(dir)
	end
	if dir == '' then
		dir = fs.pwd()
	end
	for _,c in ipairs(components) do
		dir = path.join(dir,c)
		local r,e = fs.mkdir(dir)
		if not r then
			return r,e
		end
	end
end

local CHUNK_SIZE = 1024*4
--- Reads an entire file into memory.
---@param fn string Path to the file to read
---@return string The entire contents of the file
function fs.load_file( fn )
	local cont = {}
	local f = assert( fs.open(fn,fs.O_RDONLY) )
	while true do
		local ch = assert(f:read(CHUNK_SIZE))
		table.insert(cont,ch)
		if #ch < CHUNK_SIZE then
			break
		end
	end
	f:close()
	return table.concat(cont,'')
end

--- Creates an iterator to read a file in chunks.
---@param fn string Path to the file to read
---@return function Iterator function that returns chunks of the file
---@return uv.file? File handle
function fs.read_file( fn )
	local cont = {}
	---@type uv.file?
	local f = assert( fs.open(fn,fs.O_RDONLY) )
	return function()
		if not f then
			return nil
		end
		local ch = assert(f:read(CHUNK_SIZE))
		if #ch < CHUNK_SIZE then
			f:close()
			f = nil
		end
		return ch
	end,f
end

--- Opens a file for writing.
---@param fn string Path to the file to open
---@return uv.file? File handle on success
---@return string? Error message if opening fails
function fs.open_write( fn )
	return fs.open(fn,fs.O_WRONLY|fs.O_CREAT)
end

--- Writes data to a file, creating it if it doesn't exist or overwriting it if it does.
---@param fn string Path to the file to write
---@param ... string One or more strings to write to the file
function fs.write_file( fn , ... )
	fs.unlink(fn)
	local f = assert(fs.open(fn,fs.O_WRONLY|fs.O_CREAT))
	f:write(...)
	f:close()
end


--- Internal function to find executable in PATH.
---@param PATH string The PATH environment variable
---@param bin string The executable name to find
---@return string? Full path to the executable if found
local function find_exe(PATH,bin)
	local from = 1
	while true do
		local e = string.find(PATH,':',from,true)
		if not e then
			break
		end
		local dir = string.sub(PATH,from,e-1)
		local exename = path.join(dir,bin)
		if fs.isfile(exename) then
			return exename
		end
		from = e + 1
	end
	local dir = string.sub(PATH,from)
	local exename = path.join(dir,bin)
	if fs.isfile(exename) then
		return exename
	end
end

--- Finds the full path to an executable by searching in PATH.
---@param bin string The executable name to find
---@return string Full path to the executable
function fs.find_exe(bin)
	if path.isabsolute(bin) then
		if not fs.isfile(bin) then
			error('not found exe ' .. bin)
		end
		return bin
	end
	local exename = find_exe(os.getenv('PATH'),bin)
	if not exename then
		error('not found exe ' .. tostring(bin))
	end
	return exename
end

return fs