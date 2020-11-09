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

function utils.run( fn , handle_error )
	local th = coroutine.create( handle_error and function() 
		local res,err = xpcall(fn,debug.traceback)
		if not res then
			error(err)
		end
	end or fn )
	local res, err = coroutine.resume( th )
	if not res then
		error(err)
	end
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
	local idx = 1
	local oidx = 1
	while true do
		local arg = args[idx]
		if not arg then
			break
		end
		local k,v = string.match(arg,'^%-%-([^%s=]+)=(.+)$')
		if k then 
			res[k] = v
		else
			res[oidx] = arg
			oidx = oidx + 1
		end
		idx = idx + 1
	end
	return res
end


return utils