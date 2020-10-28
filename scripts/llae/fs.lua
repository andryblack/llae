local uv = require 'uv'
local path = require 'llae.path'

local fs = setmetatable({},{__index=uv.fs})

function fs.home(  )
	return '${HOME}'
end

function fs.pwd(  )
	return '${PWD}'
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


return fs