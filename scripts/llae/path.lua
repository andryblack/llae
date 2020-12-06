
local path = {}

local function findlast(s, pattern, plain)
	local curr = 0
	repeat
		local next = s:find(pattern, curr + 1, plain)
		if (next) then curr = next end
	until (not next)
	if (curr > 0) then
		return curr
	end
end

function path.join( ... )
	return table.concat( table.pack(...) , '/' )
end

function path.basename( path )
	local i = findlast(path,"[/\\]")
	if i then
		return string.sub(path,i + 1)
	else
		return path
	end
end

function path.dirname( path )
	local i = findlast(path,"[/\\]")
	local r = i and string.sub(path,1,i-1) or ''
	if path:sub(1,1) == '/' then
		if r == '' then
			r = '/'
		end
	end
	return r
end

function path.extension( path )
	local i = findlast(path,".",true)
	return i and string.sub(path,i+1)
end

function path.getabsolute( fn )
	if string.sub(fn,1,1) == '/' then
		return fn
	end
	local fs = require 'llae.fs'
	return path.join(fs.pwd(),fn)
end

function path.getrelative( fn )
	if string.sub(fn,1,1) ~= '/' then
		return fn
	end
	local fs = require 'llae.fs'
	local pwd = fs.pwd()
	local prepfn = string.sub(fn,1,#pwd)
	if prepfn == pwd then
		return string.sub(fn,#pwd+2) -- '/'
	end
	return fn
end

return path