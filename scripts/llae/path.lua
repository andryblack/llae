local path = {}

---@param p string
---@return string
function path.normalize(p)
	local res = p:gsub("[/\\]+", "/")
	res = res:gsub("^%./", "")
	res = res:gsub("/%./", "/")
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

---@param s string
---@param pattern string
---@param plain boolean?
---@return integer?
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

---@param p string
---@return string[]
function path.split(p)
	local parts = {}
	for part in path.normalize(p):gmatch('[^/\\]+') do
		table.insert(parts, part)
	end
	return parts
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
function path.dirname( p )
	p = path.normalize(p)
	local i = findlast(p,"[/\\]")
	local r = i and string.sub(p,1,i-1) or ''
	if p:sub(1,1) == '/' then
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
	if not path.isabsolute(fn) then
		return fn
	end
	if not to then
		local fs = require 'llae.fs'
		to = fs.pwd()
	end
	fn = fn:gsub('[/\\]+$', '')
	to = to:gsub('[/\\]+$', '')

	local fn_parts = path.split(fn)
	local to_parts = path.split(to)

	local common = 0
	local min_len = math.min(#fn_parts, #to_parts)
	for i = 1, min_len do
		if fn_parts[i] == to_parts[i] then
			common = i
		else
			break
		end
	end

	-- no shared ancestor — return original absolute path
	if common == 0 then
		return fn
	end

	local result = {}
	for _ = common + 1, #to_parts do
		table.insert(result, '..')
	end
	for i = common + 1, #fn_parts do
		table.insert(result, fn_parts[i])
	end

	if #result == 0 then
		return '.'
	end
	return table.concat(result, '/')
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