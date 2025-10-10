local llae = require 'llae'
local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'llae.uv'
local log = require 'llae.log'
local async = require 'llae.async'

---The redis module provides an asynchronous Redis client implementation using coroutines.
---It supports all standard Redis commands, pipelining, and pub/sub functionality.
---@class db.redis
---@field new fun(): db.redis
local redis = class(nil,'db.redis')

redis.resp = require 'db.redis.resp'

function redis:_init( )
	self._lock = async.lock.new()
	self._subscribe = {}
	self._unsubscribe = {}
	self._subhandlers = {}
end

--print('122')

--- Connects to Redis server using TCP or Unix socket.
---@param addr string|number Redis server address (IP address for TCP, or Unix socket path)
---@param port number? TCP port number (if not provided, assumes Unix socket)
---@return boolean? True on success
---@return string? Error message if connection fails
function redis:connect( addr , port )
	if self._conn then
		return false, 'already'
	end
	if not port then
		self._conn = uv.pipe.new()
		self._data = ''
		return self._conn:connect(addr)
	else
		self._conn = uv.tcp_connection.new()
		self._data = ''
		return self._conn:connect(addr,port)
	end
end

--- Closes the Redis connection.
function redis:close(  )
	self._lock:lock()
	if self._conn then
		self._conn:shutdown()
		self._conn = nil
	end
	self._lock:unlock()
end

local function read_line( self )
	local i = 1
	while true do
		local rn = self._data:find('\r\n',i,true)
		if rn then
			local line = self._data:sub(1,rn-1)
			self._data = self._data:sub(rn+2)
			return tostring(line)
		end

		i = math.max(1,#self._data-2)

		local ch,e = self._conn:read()
		if not ch then
			return nil,e
		end
		--print(ch)
		self._data = self._data .. ch
		self._conn:add_read_buffer(ch)
	end
end

local marker_bulk,marker_simple,marker_array,marker_number,marker_error = string.byte('$+*:-',1,5)

local function read_reply( self )
	local data,e = read_line(self)
	if not data then
		return nil,e
	end
	local marker = string.byte(data,1)
	if marker == marker_bulk then
		local len = tonumber(string.sub(data,2))
		if len < 0 then
			return nil
		end
		local d,err = read_line(self)
		if not d then
			return nil,err
		end
		while #d < len do
			local ch,err = read_line(self)
			if not ch then
				return nil,err
			end
			d = d .. '\r\n' .. ch
		end
		return d
	elseif marker == marker_simple then
		return string.sub(data,2)
	elseif marker == marker_array then
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
	elseif marker == marker_number then
		return tonumber(string.sub(data,2))
	elseif marker == marker_error then
		return false, string.sub(data,2)
	else
		return false ,'unexpected'..marker
	end
end


--- Executes a Redis command and throws an error if it fails.
---@param ... any Redis command name and arguments
---@return any Command result
function redis:cmd(  ... )
	if not self._conn then
		error('closed')
	end
	self._lock:lock()
	local req = redis.resp.gen_req(...)
	--print('send: ',req)
	local res,err = self._conn:write(req)
	if not res then
		self._lock:unlock()
		error(err or 'unknown')
	end
	res,err = read_reply(self)
	self._lock:unlock()
	if err then
		error(err)
	end
	return res
end

--- Executes a Redis command and returns error instead of throwing.
---@param ... any Redis command name and arguments
---@return any? Command result or nil on error
---@return string? Error message if command failed
function redis:try_cmd(  ... )
	if not self._conn then
		return nil,'closed'
	end
	self._lock:lock()
	local req = redis.resp.gen_req(...)
	--print('send: ',req)
	local res,err = self._conn:write(req)
	if not res then
		self._lock:unlock()
		return nil,err
	end
	res,err = read_reply(self)
	self._lock:unlock()
	return res,err
end

--- Executes multiple commands in a pipeline.
---@param encoded_commands string[] Array of encoded Redis commands
---@return any[]? Array of command results on success
---@return string? Error message if pipeline failed
function redis:pipelining( encoded_commands )
	self._lock:lock()
	--print('send: ',req)
	local res,err = self._conn:write(encoded_commands)
	if not res then
		self._lock:unlock()
		return nil,err
	end
	local result = {}
	for _,v in ipairs(encoded_commands) do
		table.insert(result,{read_reply(self)})
	end
	self._lock:unlock()
	return result
end

--- Encodes Redis command arguments into RESP format.
---@param ... any Command arguments
---@return string Encoded command in RESP format
function redis.encode( ... )
	return redis.resp.gen_req(...)
end

function redis:pubsubcmd( ...)
	if not self._conn then
		return nil,'closed'
	end
	local req = redis.resp.gen_req(...)
	--print('send: ',req)
	local res,err = self._conn:write(req)
	return res,err
end

local commands = {
	'get','set','del',
	'incr','decr',
	'mget','mset',
	'llen','lindex','lpop','rpop','lpush','rpush','lrange','linsert','ltrim','lrem','lset',
	'hexists','hget','hgetall','hset','hsetnx',
	'hmget','hmset','hdel','hincrby','hkeys','hlen',
	'hstrlen','hvals',
	'smembers','sismember','sadd','srem','sdiff','sinter','sunion','srandmember',
	'zrange','zrangebyscore','zrank','zadd','zrem','zincrby',
	'auth','eval','script','sort','scan',
	'expire','persist',
	'publish',
	'rename',
	'bitfield','bitcount','bitop','bitpos','getbit','setbit'
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
	return res,err
end

function redis:try_start_subscribe()
	if self._sub_thread then
		return true
	end
	self._sub_thread = coroutine.create(function(this)
		--print('start sub')
		while next(this._subscribe) or next(this._subhandlers) or next(this._unsubscribe) do
			--print('sub read')
			local data,err = read_reply(self)
			if not data then
				error(err)
				return false,err
			end
			--print('sub',data[1],data[2],data[3])
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
					llae.resume(th,true)
				else
					log.error('redis','not found',data[2])
				end
			elseif data[1] == 'subscribe' then
				local th = self._subscribe[data[2]]
				if th then
					self._subscribe[data[2]] = nil
					--print('resume subscriber:',th)
					llae.resume(th,true)
				else
					log.error('redis','not found',data[2])
				end
			end
		end
		this._sub_thread = nil
	end)
	llae.resume(self._sub_thread,self)
end

--- Subscribes to one or more Redis channels.
---@param ... string|function Channel names and handler function (handler should be the last argument)
---@return boolean? True on success
---@return string? Error message if subscription fails
function redis:subscribe( ... )
	local args = {...}
	local handler = table.remove(args,#args)
	local channels = {}
	for _,v in ipairs(args) do
		channels[v] = handler
	end
	if not handler or not next(channels) then
		error('invalid arguments')
	end
	self._lock:lock()
	table.insert(args,1,'SUBSCRIBE')
	local req = redis.resp.gen_req(table.unpack(args))
	--print('send: ',req)
	local res,err = self._conn:write(req)
	if not res then
		self._lock:unlock()
		error(err or 'unknown')
	end

	local cor = assert(coroutine.running())

	for k,v in pairs(channels) do
		--print('++subscribe : ',k,v)
		self._subhandlers[k]=v
		self._subscribe[k]=cor
	end
	self:try_start_subscribe()
	
	while next(self._subscribe) do
		--print('>wait subscibe',cor)
		coroutine.yield(cor)
		--print('<wait subscibe')
	end

	self._lock:unlock()
	return true
end

return redis