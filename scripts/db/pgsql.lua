local llae = require 'llae'
local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'llae.uv'
local log = require 'llae.log'
local json = require 'llae.json'

local pgsql = class(nil,'db.pgsql')

for n,v in pairs(require 'db.pgsql.types') do
	pgsql[n]=v
end

pgsql.default_config = {
	database = 'postgres',
	user = 'postgres',
	host = '127.0.0.1',
	port = 5432,
	ssl = false
}

function pgsql:_init(conf)
	self._config = conf or self.default_config
	self._lock = uv.lock.new()
	self._conn = uv.tcp_connection.new()
end

function pgsql:connect()
	self._lock:lock()
	self._data = ''
	local res,err = self._conn:connect(self._config.host,self._config.port)
	if not res then
		self._lock:unlock()
		return res,err
	end
	res,err = self:send_startup_message()
	if not res then
		self._lock:unlock()
		return res,err
	end
	res,err = self:auth()
	if not res then
		self._lock:unlock()
		return res,err
	end
	res,err = self:wait_until_ready()
	if not res then
		self._lock:unlock()
		return res,err
	end
	self._lock:unlock()
	return true
end

function pgsql:read(len)
	while #self._data < len do
		local ch,err = self._conn:read()
		if not ch then
			if not err then
				log.debug('psql','connection closed')
			end
			return ch,err
		end
		self._data = self._data .. ch
	end
	local r = self._data:sub(1,len)
	self._data = self._data:sub(len+1)
	return r
end

function pgsql:disconnect()
	self._lock:lock()
	self:send_message(self.message_type_f.terminate,{})
	self._conn:shutdown()
	self._conn:close()
	self._lock:unlock()
end

function pgsql:auth()
	local t,msg = self:receive_message()
	if not t then
		return t,msg
	end
	if t ~= self.message_type_b.auth then
		if t == self.message_type_b.error then
			return nil, self:parse_error(msg)
		end
		return nil,'need auth message'
	end
	local auth_type = string.unpack('>I4',msg)
	if auth_type == 0 then
		return true
	elseif auth_type == 3 then
		return self:cleartext_auth(msg)
	elseif auth_type == 5 then
		return self:md5_auth(msg)
	else
		return nil,'usupported auth method'
	end
end

function pgsql:cleartext_auth(msg)
	if not self._config.password then
		return nil,'need password for auth'
	end
	self:send_message(self.message_type_f.password,{
		self._config.password,
		self.NULL
	})
	return self:check_auth()
end

function pgsql:check_auth()
	local t, msg = self:receive_message()
	if not t then
		return nil,msg
	end
	if t == self.message_type_b.error then
		return nil, self:parse_error(msg)
	elseif t == self.message_type_b.auth then
		return true
	else
		return nil, 'invalid auth response'
	end
end

function pgsql:receive_message()
	local ch,err = self:read(5)
	if not ch then
		return nil,err
	end
	local t,len = string.unpack('>I1I4',ch)
	len = len - 4
	local msg,err = self:read(len)
	if not msg then
		return nil,err
	end
	return t,msg
end

function pgsql:send_message(t,smsg)
	local msg = self:encode(smsg)
	local data = string.pack('>I1I4',t,#msg + 4) .. msg
	return self._conn:write(data)
end

function pgsql:encode(msg)
	local r = {}
	for _,v in ipairs(msg) do
		if type(v) == 'string' then
			table.insert(r,v)
		else
			error('unexpected data')
		end
	end
	return table.concat(r,'')
end

function pgsql:send_startup_message()
	if not self._config.user then
		return nil,'need user for connect'
	end
	if not self._config.database then
		return nil,'need database for connect'
	end
	local data = {
		string.pack('>I4',196608),
		'user',self.NULL,
		self._config.user,self.NULL,
		'database',self.NULL,
		self._config.database,self.NULL,
		'application_name',self.NULL,
		(self._config.application_name or 'llae-client'),self.NULL,
		self.NULL
	}
	data = self:encode(data)
	return self._conn:write(string.pack('>I4',#data+4)..data)
end

function pgsql:wait_until_ready()
	while true do
		local t,msg = self:receive_message()
		if not t then
			return nil,msg
		end
		if t == self.message_type_b.error then
			return nil,self:parse_error(msg)
		end
		if t == self.message_type_b.ready_for_query then
			break
		end
	end
	return true
end

function pgsql:parse_error(err_msg)
	local offset = 1
	local error_data = {}
	while offset <= #err_msg do
		local t = string.unpack('>I1',err_msg,offset)
		local str = err_msg:match("[^%z]+", offset + 1)
		if not str then
			break
		end
		offset = offset + 1 + #str + 1
		local field = self.pg_error[t]
		if field then
			error_data[field] = str
		end
	end
	local msg = tostring(error_data.severity) .. ':' .. tostring(error_data.message)
	if error_data.position then
		msg = msg .. ' (' .. tostring(error_data.position) .. ')'
	end
	if error_data.detail then
		msg = msg .. '\n' .. tostring(error_data.detail)
	end
	return msg,error_data
end

function pgsql:parse_row_desc(row_desc)
	local num_fields = string.unpack('>I2',row_desc)
	local offset = 3
	local fields = {}
	for i=1,num_fields do
		local name = row_desc:match("[^%z]+",offset)
		offset = offset + #name + 1
		local data_type = string.unpack('>I4',row_desc,offset+6)
		data_type = self.pg_type[data_type] or 'string'
		local format = string.unpack('>I2',row_desc,offset+16)
		assert(0 == format, "don't know how to handle format " .. tostring(format))
		offset = offset + 18
		table.insert(fields,{name,data_type})
	end
	return fields
end

pgsql.type_deserializers = {}


--local decode_array = require("db.pgsql.arrays").decode_array
--local decode_hstore = require("db.pgsql.hstore").decode_hstore

function pgsql.type_deserializers:json(val, name)
	return json.decode(val,true)
end

function pgsql.type_deserializers:bytea(val, name)
	return self:decode_bytea(val)
end
-- function pgsql.type_deserializers:array_boolean(val, name)
-- 	return decode_array(val,function(v) return v=='t' end,self)
-- end
-- function pgsql.type_deserializers:array_number(val, name)
-- 	return decode_array(val,tonumber,self)
-- end
-- function pgsql.type_deserializers:array_string(val, name)
-- 	return decode_array(val,tostring,self)
-- end
-- function pgsql.type_deserializers:array_json(val, name)
-- 	return decode_array(val,decode_json,self)
-- end
-- function pgsql.type_deserializers:hstore(val, name)
-- 	return decode_hstore(val,decode_json,self)
-- end


function pgsql:parse_data_row(data_row,fields)
	local num_fields = string.unpack('>I2',data_row)
	local offset = 3
	local out = {}
	for i=1,num_fields do
		local field = fields[i]
		if not field then
			break
		end
		local field_name,field_type = field[1],field[2]
		local len = string.unpack('>I4',data_row,offset)
		offset = offset + 4
		if len < 0 then
			if self.convert_null then
				out[field_name] = self.NULL
			else
				break
			end
		end
		local value = data_row:sub(offset,offset+len-1)
		offset = offset + len
		if field_type == 'number' then
			value = tonumber(value)
		elseif field_type == 'boolean' then
			value = value == 't'
		elseif field_type == 'string' then
			value = tostring(value)
		else
			local fn = self.type_deserializers[field_type]
			if fn then
				value = fn(self, value, field_type)
			end
		end
		out[field_name] = value
	end
	return out
end

function pgsql:format_query_result(row_desc,data_rows,command_complete)
	local command,affected_rows
	if command_complete then
		command = command_complete:match('^%w+')
		affected_rows = tonumber(command_complete:match('(%d+)%z$'))
	end
	if row_desc then
		if not data_rows then
			return {}
		end
		local fields = self:parse_row_desc(row_desc)
		local num_rows = #data_rows
		for i = 1, num_rows do
          data_rows[i] = self:parse_data_row(data_rows[i], fields)
        end
        if affected_rows and command ~= "SELECT" then
          data_rows.affected_rows = affected_rows
        end
        return data_rows
	end
	if affected_rows then
        return {
          	affected_rows = affected_rows
        }
    else
       return true
    end
end

function pgsql:receive_query_result()
	local row_desc, data_rows, command_complete, err_msg
	local result, notifications, notices
    local num_queries = 0
    while true do
    	local t, msg = self:receive_message()
        if not t then
          return nil, msg
        end
        if t == self.message_type_b.data_row then
        	if not data_rows then
        		data_rows = {}
        	end
        	table.insert(data_rows,msg)
        elseif t == self.message_type_b.row_description then
          	row_desc = msg
        elseif t == self.message_type_b.error then
        	err_msg = msg
        elseif t == self.message_type_b.notice then
        	if not notices then
        		notices = {}
        	end
        	table.insert(notices,self:parse_error(msg))
        elseif t == self.message_type_b.command_complete then
        	command_complete = msg
        	local next_result = self:format_query_result(row_desc,data_rows,command_complete)
        	num_queries = num_queries + 1
        	if num_queries == 1 then
        		result = next_result
        	elseif num_queries == 2 then
        		result = {
        			result,
        			next_result
        		}
        	else
        		table.insert(result,next_result)
        	end
        	row_desc, data_rows, command_complete = nil
        elseif t == self.message_type_b.ready_for_query then
        	break
        elseif t == self.message_type_b.notification then
        	if not notifications then
        		notifications = {}
        	end
        	table.insert(notifications,self:parse_notification(msg))
        elseif t == self.message_type_b.parse_complete or
        	t == self.message_type_b.bind_complete or
        	t == self.message_type_b.close_complete then
        else
        	log.error("Unhandled message in query result: " , tostring(t))
        end
    end
    if err_msg then
		return nil, self:parse_error(err_msg), result, num_queries, notifications, notices
	end
	return result, num_queries, notifications, notices
end

function pgsql:simple_query(q)
	if q:find(self.NULL) then
       return nil, "invalid null byte in query"
    end
	self._lock:lock()
	local res,err = self:send_message(self.message_type_f.query,{q,self.NULL})
	if not res then
		self._lock:unlock()
		return nil,err
	end
	res,err = self:receive_query_result()
	self._lock:unlock()
	return res,err
end

function pgsql:query(q)
	return self:simple_query(q)
end

function pgsql:decode_bytea(str)
	if str:sub(1, 2) == '\\x' then
		return str:sub(3):gsub('..', function(hex)
		  return string.char(tonumber(hex, 16))
		end)
	else
		return str:gsub('\\(%d%d%d)', function(oct)
		  return string.char(tonumber(oct, 8))
		end)
	end
end
function pgsql:encode_bytea(str)
    return string.format("E'\\\\x%s'", str:gsub('.', function(byte)
        return string.format('%02x', string.byte(byte))
    end))
end

return pgsql