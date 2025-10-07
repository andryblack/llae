local utils = {}

---@param t table<any,any>
function utils.clear_table( t )
	while next(t) do
		local k,v = next(t)
		t[k] = nil
	end
end

---@param str string
---@return string?
function replace_env_i( str )
	local env = string.match(str,'^{(.+)}$')
	if env then
		return os.getenv(env)
	end
end

---@param str string
---@return string
function utils.replace_env( str )
	local res = string.gsub(str,'%$(%b{})',replace_env_i)
	return res
end

---@param text string
---@param tokens table<string,string>
function utils.replace_tokens( text, tokens )
	local res = string.gsub(text,'%$(%b{})',function(str)
		local env = string.match(str,'^{(.+)}$')
		if env then
			return tokens[env]
		end
	end)
	return res
end

---parse command line args
---@param args table<string|integer,string>
---@return table<string|integer,string|true>
function utils.parse_args( args )
	---@type table<string|integer,string|true>
	local res = {[0]=args[0]}
	for k,v in pairs(args) do
		if type(k) == 'string' then
			res[k]=v
		end
	end
	local idx = 1
	local oidx = 1
	while true do
		local arg = args[idx]
		if not arg then
			break
		end
		local opt = string.match(arg,'^%-%-(.+)$')
		if opt then
			local k,v = string.match(opt,'^([^%s=]+)=(.+)$')
			if k then 
				res[k] = v
			else
				res[opt] = true
			end
		else
			res[oidx] = arg
			oidx = oidx + 1
		end
		idx = idx + 1
	end
	return res
end

---merge tables
---@param ... table<any,any>
---@return table<any,any>
function utils.merge( ... )
	---@type table<any,any>
	local r = {}
	local t = table.pack(...)
	for i=1,t.n do
		---@type table<any,any>
		local at = t[i]
		if at then
			for kt,kv in pairs(at) do
				r[kt]=kv
			end
		end
	end
	return r
end

---concatenate 2 list
---@param a any[]
---@param b any[]
---@return any[]
function utils.list_concat( a,b )
	local r = {}
	for _,v in ipairs(a) do table.insert(r,v) end
	for _,v in ipairs(b) do table.insert(r,v) end
	return r
end

---@param t any[]
---@param i integer
local function reversedipairsiter(t, i)
    i = i - 1
    if i ~= 0 then
        return i, t[i]
    end
end

---@param t any[]
---@return fun(t:any[],i:integer):integer,any
---@return any[]
---@return integer
function utils.reversedipairs(t)
    return reversedipairsiter, t, #t + 1
end

return utils