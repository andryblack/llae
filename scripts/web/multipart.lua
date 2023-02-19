local class = require 'llae.class'
local url = require 'net.url'
local uv = require 'llae.uv'
local log = require 'llae.log'
local headers = require 'net.http.headers'

local multipart = class(require 'web.middle','multipart')

function multipart.parse(str,bound)
	local pos = 1
	local parts = {}
	local prefix = '--' .. bound
	--print('parse',#str,bound)
	while pos < #str do
		local bpos = string.find(str,prefix,pos,true)
		if not bpos then
			log.error('not found start bound')
			break
		end
		bpos = bpos + #prefix
		local estr = str:sub(bpos,bpos+1)
		if estr == '--' then
			log.debug('found end bound')
			break
		end
		if estr ~= '\r\n' then
			--print('['..estr..']')
			log.error('invalid bound')
			break
		end
		pos = bpos + 2
		local part = {headers=headers.new()}
		--log.debug('---parse-headers---')
		while pos < #str do
			local eline = string.find(str,'\r\n',pos,true)
			if not eline then
				pos = #str
				break
			end
			local line = str:sub(pos,eline-1)
			pos = eline+2
			if line == '' then
				break
			else
				local name,value = string.match(line,'^([^:]+)%s*:%s*(.*)$')
				if name then
					part.headers:set_header(name,value)
				end
			end
		end
		local cdisp = part.headers:get_header('Content-Disposition')
		if cdisp then
			for k, v in string.gmatch(cdisp, ';%s*(%w+)%s*=%s*"([^"]*)"') do
				part[k]=v
		    end
		end
		local epos = string.find(str,'\r\n'..prefix,pos,true)
		if epos then
			--print('---found body---',pos,epos)
			part.data = str:sub(pos,epos-1)
			pos = epos
		else
			log.error('not found multipart terminator')
			break
		end
		table.insert(parts,part)
	end
	return parts
end


function multipart.build(values)
	local rand_data = string.pack('<I4I4I4I4',math.random(1,0x7fffffff),math.random(1,0x7fffffff),math.random(1,0x7fffffff),math.random(1,0x7fffffff))
	local bounds = '--------------' .. uv.buffer.hex_encode(rand_data)
	local prefix = '--' .. bounds
	local res = {}
	for _,v in ipairs(values) do
		table.insert(res,prefix)
		local hdrs = headers.new(v.headers)
		if not hdrs:get_header('Content-Disposition') then
			local disp = {'form-data'}
			if v.name then
				table.insert(disp,'name="' .. v.name .. '"')
			end
			if v.filename then
				table.insert(disp,'filename="' .. v.filename .. '"')
			end
			hdrs:set_header('Content-Disposition',table.concat(disp,'; '))
		end
		for n,v in headers:foreach_header() do
			table.insert(res,n..': ' .. v)
		end
		table.insert(res,'')
		table.insert(res,v.data)
	end
	table.insert(res,prefix .. '--')
	return table.concat(res,'\r\n'),bounds
end

function multipart:handle_request(req,res)
	local ct = req:get_header('Content-Type')
	if ct then
		--print(ct)
		local bound = string.match(ct,'multipart/form%-data;%s*boundary=(.+)')
		if bound then
			--print(bound)
			req.body = req.body or req:read_body()
			req.multipart = multipart.parse(req.body,bound)
		end
	end
end

function multipart:use(web)
	web:register_handler(function(req,res)
		self:handle_request(req,res)
	end)
end


return multipart
