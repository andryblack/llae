local path = {}

---@param p string
---@return string
function path.normalize(p)
	local res = p:gsub("[/\\]+", "/")
	return res
end

---@param p string
---@return boolean
function path.isabsolute(p)
	if string.sub(p,1,1) == '/' then
		return true
	end
	-- Check for Windows drive letter path (e.g. C:/ or C:\)
	if string.match(p, '^%a%:[\\/]') then
		return true
	end
	-- Windows UNC path
	if string.sub(p,1,1) == '\\' then
		return true
	end
	return false
end

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

---@param ... string
---@return string
function path.join( ... )
	return table.concat( table.pack(...) , '/' )
end

---@param path string
---@return string
function path.basename( path )
	local i = findlast(path,"[/\\]")
	if i then
		return string.sub(path,i + 1)
	else
		return path
	end
end

---@param path string
---@return string
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

---@param path string
---@return string?
function path.extension( path )
	local i = findlast(path,".",true)
	return i and string.sub(path,i+1)
end

---@param fn string
---@return string
function path.getabsolute( fn )
	if path.isabsolute(fn) then
		return fn
	end
	local fs = require 'llae.fs'
	return path.join(fs.pwd(),fn)
end

---@param fn string
---@param to string?
---@return string
function path.getrelative( fn, to )
	if string.sub(fn,1,1) ~= '/' then
		return fn
	end
	if not to then
		local fs = require 'llae.fs'
		to = fs.pwd()
	end
	local prepfn = string.sub(fn,1,#to)
	if prepfn == to then
		return string.sub(fn,#to+2) -- '/'
	end
	return fn
end

---@param fn string
---@param count number
---@return string?
function path.remove_leading_dirs(fn,count)
	local i = 1
	for j = 1,count do
		local p = string.find(fn,'/',i,true)
		if not p then
			return nil
		end
		i = p + 1
	end
	return string.sub(fn,i)
end

return path