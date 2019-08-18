local llae = require 'llae'
local file = require 'llae.file'

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


return path