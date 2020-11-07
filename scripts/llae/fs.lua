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