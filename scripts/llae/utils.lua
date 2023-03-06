local utils = {}

function utils.clear_table( t )
	while next(t) do
		local k,v = next(t)
		t[k] = nil
	end
end

function replace_env_i( str )
	local env = string.match(str,'^{(.+)}$')
	if env then
		return os.getenv(env)
	end
end
function utils.replace_env( str )
	local res = string.gsub(str,'%$(%b{})',replace_env_i)
	return res
end

function utils.replace_tokens( text, tokens )
	local res = string.gsub(text,'%$(%b{})',function(str)
		local env = string.match(str,'^{(.+)}$')
		if env then
			return tokens[env]
		end
	end)
	return res
end

function utils.parse_args( args )
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

function utils.merge( ... )
	local r = {}
	local t = table.pack(...)
	for i=1,t.n do
		local at = t[i]
		if at then
			for kt,kv in pairs(at) do
				r[kt]=kv
			end
		end
	end
	return r
end

function utils.list_concat( a,b )
	local r = {}
	for _,v in ipairs(a) do table.insert(r,v) end
	for _,v in ipairs(b) do table.insert(r,v) end
	return r
end


local function reversedipairsiter(t, i)
    i = i - 1
    if i ~= 0 then
        return i, t[i]
    end
end

function utils.reversedipairs(t)
    return reversedipairsiter, t, #t + 1
end

return utils