local llae = require 'llae'
local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'llae.uv'

local redis = class(nil,'db.redis')


function redis:_init( )
	self._conn = uv.tcp_connection.new()
	self._lock = uv.lock.new()
	self._subscribe = {}
	self._unsubscribe = {}
	self._subhandlers = {}
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
			print(e)
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
	else
		return false ,'unexpected'..prefix
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

function redis:cmd(  ... )
	self._lock:lock()
	local args = {...}
	local req = gen_req(args)
	--print('send: ',req)
	local res,err = self._conn:write{req,'\r\n'}
	if not res then
		self._lock:unlock()
		return nil,err
	end
	res,err = read_reply(self)
	self._lock:unlock()
	return res,err
end

function redis:pubsubcmd( ...)
	local args = {...}
	local req = gen_req(args)
	--print('send: ',req)
	local res,err = self._conn:write{req,'\r\n'}
	return res,err
end

local commands = {
	'get','set','del',
	'incr','decr',
	'mget','mset',
	'llen','lindex','lpop','lpush','lrange','linsert',
	'hexists','hget','hgetall','hset','hsetnx',
	'hmget','hmset','hdel','hincrby','hkeys','hlen',
	'hstrlen','hvals',
	'smembers','smismember','sadd','srem','sdiff','sinter','sunion',
	'zrange','zrangebyscore','zrank','zadd','zrem','zincrby',
	'auth','eval','script','sort','scan',
	'expire','persist',
	'publish',
}

for _,cmd in ipairs(commands) do
	local ucmd = string.upper(cmd)
	redis[cmd] = function(self,...)
		return redis.cmd(self,ucmd,...)
	end
end

function redis:unsubscribe(...)
	local res,err = self:pubsubcmd('UNSUBSCRIBE',...)
	local cor = assert(coroutine.running())
	local channels = {...}
	if not next(channels) then
		for k,v in pairs(self._subhandlers) do
			self._unsubscribe[k] = cor
		end
	else
		for _,v in pairs(channels) do
			self._unsubscribe[v] = cor
		end
	end
	while next(self._unsubscribe) do
		coroutine.yield(cor)
	end
	return true
end

function redis:try_start_subscribe()
	if self._sub_thread then
		return true
	end
	self._sub_thread = coroutine.create(function(this)
		--print('start sub')
		while next(this._subscribe) or next(this._subhandlers) or next(this._unsubscribe) do
			local data,err = read_reply(self)
			if not data then
				return false,err
			end
			--print('sub',data[1],data[2],data[3],e)
			if data[1] == 'message' then
				local handler = self._subhandlers[data[2]]
				if handler then
					handler(data[2],data[3])
				end
			elseif data[1] == 'unsubscribe' then
				local th = self._unsubscribe[data[2]]
				if th then
					self._unsubscribe[data[2]] = nil
					self._subhandlers[data[2]] = nil
					assert(coroutine.resume(th,true))
				else
					print('not found',data[2])
				end
			elseif data[1] == 'subscribe' then
				local th = self._subscribe[data[2]]
				if th then
					self._subscribe[data[2]] = nil
					assert(coroutine.resume(th,true))
				else
					print('not found',data[2])
				end
			end
		end
	end)
	assert(coroutine.resume(self._sub_thread,self))
end

function redis:subscribe( ... )
	local args = {...}
	local handler = table.remove(args,#args)
	local channels = {}
	for _,v in ipairs(args) do
		channels[v] = handler
	end
	if not handler or not next(channels) then
		return nil, 'invalid arguments'
	end
	self._lock:lock()
	table.insert(args,1,'SUBSCRIBE')
	local req = gen_req(args)
	--print('send: ',req)
	local res,err = self._conn:write{req,'\r\n'}
	if not res then
		self._lock:unlock()
		return nil,err
	end

	local cor = assert(coroutine.running())

	for k,v in pairs(channels) do
		--print('++subscribe : ',k,v)
		self._subhandlers[k]=v
		self._subscribe[k]=cor
	end
	self:try_start_subscribe()
	
	while next(self._subscribe) do
		--print('>wait subscibe')
		coroutine.yield(cor)
		--print('<wait subscibe')
	end

	self._lock:unlock()
	return true
end

return redis