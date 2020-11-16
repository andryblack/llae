local uv = require 'uv'
local path = require 'llae.path'

local fs = setmetatable({},{__index=uv.fs})

fs.home = uv.os.homedir
fs.pwd = uv.cwd
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

function fs.write_file( fn , ... )
	fs.unlink(fn)
	local f = assert(fs.open(fn,fs.O_WRONLY|fs.O_CREAT))
	f:write(...)
	f:close()
end

return fs