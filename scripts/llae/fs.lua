local uv = require 'uv'
local path = require 'llae.path'

local fs = setmetatable({},{__index=uv.fs})

fs.home = uv.os.homedir
fs.pwd = uv.cwd
fs.cwd = uv.cwd
fs.chdir = uv.chdir
fs.exepath = uv.exepath

function fs.isfile( fn )
	local st = fs.stat(fn)
	return st and st.isfile
end

function fs.isdir( fn )
	local st = fs.stat(fn)
	return st and st.isdir
end

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

function fs.open( filename, mode )
	local f,er = uv.fs.open(filename,mode)
	if not f then
		return nil,'failed open ' .. filename .. ' ' .. tostring(er)
	end
	return f,er
end

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

function fs.scanfiles_r( dir )
	local res = {}
	return scanfiles_r(res,dir,'')
end


function fs.mkdir_r(dir)
	local components = {}
	while not fs.isdir(dir) do
		local f = path.basename(dir)
		if not f then
			break
		end
		dir = path.dirname(dir)
		table.insert(components,1,f)
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

function fs.read_file( fn )
	local cont = {}
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

function fs.open_write( fn )
	return fs.open(fn,fs.O_WRONLY|fs.O_CREAT)
end

function fs.write_file( fn , ... )
	fs.unlink(fn)
	local f = assert(fs.open(fn,fs.O_WRONLY|fs.O_CREAT))
	f:write(...)
	f:close()
end


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