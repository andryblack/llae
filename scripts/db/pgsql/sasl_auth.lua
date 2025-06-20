local class = require 'llae.class'
local uv = require 'llae.uv'
local log = require 'llae.log'
local llae = require 'llae'
local crypto = require 'llae.crypto'

local sasl_auth = class()

function sasl_auth:_init(config)
	self._config = config
end

function sasl_auth:create_client_first_message()
	local rand_str = uv.random(32)
	self._sasl_nonce = tostring(llae.buffer.hex_encode(rand_str))
	self._client_first_message_bare = 'n=' .. self._config.user .. ',r='..self._sasl_nonce
	self._client_first_message = 'n,,' .. self._client_first_message_bare
	return self._client_first_message
end

function sasl_auth:receive_auth_message(con,need_auth_type)
	local t,msg = con:receive_message()
	if not t then
		return t,msg
	end
	if t ~= con.message_type_b.auth then
		if t == con.message_type_b.error then
			log.error('SASL: auth error')
			return nil, con:parse_error(msg)
		end
		return nil,'need auth continue message'
	end
	local auth_type = string.unpack('>I4',msg)
	if auth_type ~= need_auth_type then
		return nil, 'invalid auth response'
	end
	return msg
end


local function sasl_parse(msg)
	local res = {}
	for str in string.gmatch(msg,'[^,]+') do
		local k,v = string.match(str,'(%l)=(.+)')
		res[k]=v
	end
	return res
end

local function xor_data(data_1,data_2)
	local data_3 = {}
	assert(#data_1 == #data_2)
	for i=1,#data_1 do
		local b1 = data_1:byte(i)
		local b2 = data_2:byte(i)
		table.insert(data_3,string.pack('I1',b1 ~ b2))
	end
	return table.concat(data_3,'')
end

local function sasl_h(msg)
	local md = assert(crypto.md.new('SHA256'))
	assert(md:update(msg))
	return assert(md:finish())
end

local function sasl_hmac(key,msg)
	local hmac = assert(crypto.hmac.new('SHA256'))
	assert(hmac:start(key))
	assert(hmac:update(msg))
	return assert(hmac:finish())
end

local function sasl_norm_passwd(password)
	return password
end

local function sasl_hi(password,salt,n)
	local str = sasl_norm_passwd(password)
	local u = sasl_hmac(str,salt .. string.pack('>I4',1))
	local res = u
	for i = 2,n do
		local u1 = sasl_hmac(str,u)
		res = xor_data(res,u1)
		u = u1
	end
	return res
end

function sasl_auth:process_server_first_message(server_first_message)
	local p = sasl_parse(server_first_message)
	local salt = tostring(llae.buffer.base64_decode(p.s))
	local salted_password = sasl_hi(self._config.password,salt,tonumber(p.i))
	local client_key = sasl_hmac(salted_password,'Client Key')
	local stored_key = sasl_h(client_key)
	local client_final_message = 'c=biws,r=' .. p.r
	local auth_message = self._client_first_message_bare .. ',' .. server_first_message .. ',' .. client_final_message
	local client_signature = sasl_hmac(stored_key,auth_message)
	local client_proof = xor_data(client_key,client_signature)
	client_final_message = client_final_message .. ',p=' .. tostring(llae.buffer.base64_encode(client_proof))
	return client_final_message
end

function sasl_auth:auth(con)
	

	local client_first_message = self:create_client_first_message()
	
	log.debug('> SASL:',client_first_message)
	-- SASLInitialResponse
	con:send_message(con.message_type_f.password,{
		"SCRAM-SHA-256",con.NULL,
		string.pack('>I4',#client_first_message),
		client_first_message
	})
	local msg,err = self:receive_auth_message(con,11)
	if not msg then
		return nil,err
	end
	local server_first_message = msg:sub(5)
	log.debug('< SASL:',server_first_message) 
	local client_final_message = self:process_server_first_message(server_first_message)
	
	-- SASLResponse
	log.debug('> SASL:',client_final_message)
	con:send_message(con.message_type_f.password,{
		client_final_message
	})
	local msg,err = self:receive_auth_message(con,12)
	if not msg then
		return nil,err
	end
	local server_final_message = msg:sub(5)
	log.debug('< SASL:',server_final_message) 
	return con:check_auth()
end

function sasl_auth.parse_mechanism(msg,config)
	local variants = {}
	local offset = 4
	while true do
		local str = msg:match("[^%z]+", offset + 1)
		if not str or #str == 0 then
			break
		end
		log.debug('SASL auth mechanism:',str)
		table.insert(variants,str)
		offset = offset + #str + 1
	end
	for _,v in ipairs(variants) do
		if v == 'SCRAM-SHA-256' then
			return sasl_auth.new(config)
		end
	end
	return nil, 'not found supported SASL auth mechanism'
end

return sasl_auth