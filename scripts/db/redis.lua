local llae = require 'llae'
local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'uv'

local redis = class(nil,'db.redis')


function redis:_init( )
	self._conn = uv.tcp_connection:new()
end

function redis:connect( addr , port )
	self._data = ''
	return self._conn:connect(addr,port)
end

function redis:close(  )
	self._conn:close()
end

local function read( self )
	while true do
		local line,tail = string.match(self._data,'^([^\r\n]*)\r\n(.*)$')
		if line then
			self._data = tail
			return line
		end

		--print('start read')
		local ch,e = self._conn:read()
		if not ch then
			--print(e)
			return nil,e
		end
		--print(ch)
		self._data = self._data .. ch
	end
end

local function read_reply( self )
	local data,e = read(self)
	if not data then
		return nil,e
	end
	local prefix = string.sub(data,1,1)
	if prefix == '$' then
		local len = tonumber(string.sub(data,2))
		if len < 0 then
			return nil
		end
		local d = read(self)
		while #d < len do
			local ch,err = read(self)
			if not ch then
				return nil,err
			end
			d = d .. '\r\n' .. ch
		end
		return d
	elseif prefix == '+' then
		return string.sub(data,2)
	elseif prefix == '*' then
		local len = tonumber(string.sub(data,2))
		if len < 0 then
			return nil
		end
		local vals = {}
		for i = 1, len do
			local v,e = read_reply(self)
			if not v then
				return nil,e
			end
			vals[i] = v
		end
		return vals
	elseif prefix == ':' then
		return tonumber(string.sub(data,2))
	elseif prefix == '-' then
		return false,string.sub(data,2)
	end
end

local function gen_req( args )
	local data = {'*' .. tostring(#args) }
	for i,v in ipairs(args) do
		local a = tostring(v)
		table.insert(data,'$' .. tostring(#a))
		table.insert(data, a )
	end
	return table.concat(data,'\r\n')
end 

local function do_cmd( self, ... )
	local args = {...}
	local req = gen_req(args)
	--print('send: ',req)
	self._conn:write{req,'\r\n'}
	return read_reply(self)
end

local function common_cmd( cmd )
	redis[cmd] = function(self,...)
		return do_cmd(self,cmd,...)
	end
end

common_cmd('get')
common_cmd('set')
common_cmd('mget')
common_cmd('mset')
common_cmd('del')
common_cmd('incr')
common_cmd('decr')

common_cmd('llen')
common_cmd('lindex')
common_cmd('lpop')
common_cmd('lpush')
common_cmd('lrange')
common_cmd('linsert')

common_cmd('hexists')
common_cmd('hget')
common_cmd('hset')
common_cmd('hmget')
common_cmd('hdel')

common_cmd('smembers')
common_cmd('sismember')
common_cmd('sadd')
common_cmd('srem')
common_cmd('sdiff')
common_cmd('sinter')
common_cmd('sunion')

common_cmd('zrange')
common_cmd('zrangebyscore')
common_cmd('zrank')
common_cmd('zadd')
common_cmd('zrem')
common_cmd('zincrby')

common_cmd('auth')
common_cmd('eval')
common_cmd('expire')
common_cmd('script')
common_cmd('sort')


return redis