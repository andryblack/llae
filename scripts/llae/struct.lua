local class = require 'llae.class'
local _M = {}

local field_def = class()

function field_def:_init(name,fdef)
	self.name = name
	if fdef then
		self._default = fdef.default
	end
end

function field_def:load(dst,src)
	local res = src[self.name] or self._default
	if not res then
		error('need field ' .. self.name)
	end
	dst[self.name] = res
end

function field_def:write(dst,src)
	self:write_data(dst,src[self.name] or error('need field ' .. self.name ))
end
function field_def:read(dst,d,o)
	assert(o)
	local data
	data,o = self:read_data(d,o)
	dst[self.name] = data
	return o
end


local array_field_def = class(field_def)


function array_field_def:_init(data,length)
	array_field_def.baseclass._init(self,data.name)
	self._data = data
	self._length = length
	self.size = length * data.size
end


function array_field_def:read_data(d,o)
	local a = {}
	local data
	for i=1,self._length do
		data,o = self._data:read_data(d,o)
		table.insert(a,data)
	end
	return a,o
end

function array_field_def:dump(data,out,o)
	local adata = data[self.name] or error('need array field ' .. self.name)
	if self._data.is_a[simple_field_def] and self._length <= 16 then
		local r = {}
		for i=1,self._length do
			table.insert(r,self._data:format(adata[i]))
		end
		out(o .. self.name,'[' .. table.concat(r,',') .. ']')
	else
		out(o .. self.name ,'[')
		local p = o .. '\t'
		for i=1,self._length do
			self._data:dump_data(adata[i],out,p)
			out(p..',')
		end
		out(o .. ']')
	end
end

function array_field_def:write_data(dst,val)
	assert(#val == self._length,'invalid array length')
	for i=1,self._length do
		self._data:write_data(dst,val[i])
	end
end

function array_field_def:write(dst,src)
	local arr =src[self.name] or error('need array field ' .. self.name )
	if type(arr) ~= 'table' then
		error('need table value for array field ' .. self.name)
	end
	if #arr ~= self._length then
		error('invalid array length ' .. tostring(#arr) .. '/' .. tostring(self._length) .. ' for field ' .. self.name)
	end
	self:write_data(dst,arr)
end

local byte_array_field_def = class(array_field_def)

function byte_array_field_def:_init(name,length)
	field_def._init(self,name)
	self._length = length
	self.size = length 
end

function byte_array_field_def:read_data(d,o)
	local data = d:sub(o+1,o+1+self._length-1)
	return data,o+self._length
end

function byte_array_field_def:write_data(r,v)
	assert(#v == self._length)
	table.insert(r,v)
end

function byte_array_field_def:write(dst,src)
	local arr =src[self.name] or error('need byte array field ' .. self.name )
	if type(arr) ~= 'string' then
		error('need string value for byte array field ' .. self.name)
	end
	self:write_data(dst,arr)
end

function byte_array_field_def:format(val)
	local r = {}
	local s = {}
	local len = math.min(#val,32)
	for i=1,len do
		local v = string.unpack('I1',val,i)
		table.insert(r,string.format('%02x',v))
		if v > 10 and v < 128 then
			table.insert(s,string.char(v))
		else
			table.insert(s,'.')
		end
	end
	if len ~= self._length then
		table.insert(r,'...')
	end
	return '[' .. table.concat(r,',') .. '](' .. table.concat(s,'') .. ')'
end

function byte_array_field_def:dump(data,out,t)
	out(t..self.name,self:format(data[self.name]))
end

local zeroterm_byte_array_field_def = class(byte_array_field_def)

function zeroterm_byte_array_field_def:read_data(d,o)
	local data,o = zeroterm_byte_array_field_def.baseclass.read_data(self,d,o)
	local p = data:find('\0',1,true)
	if p then
		data = data:sub(1,p-1)
	end
	return data,o
end

function zeroterm_byte_array_field_def:write_data(r,val)
	if #val < self._length then
		val = val .. string.rep('\0',self._length-#val)
	end
	return zeroterm_byte_array_field_def.baseclass.write_data(self,r,val)
end


local data_types = {
	u64 = {
		pack = 'I8',
		size = 8,
		format = '0x%016x'
	},
	u32 = {
		pack = 'I4',
		size = 4,
		format = '0x%08x'
	},
	u24 = { -- non standart
		pack = 'I3',
		size = 3,
		format = '0x%06x'
	},
	u16 = {
		pack = 'I2',
		size = 2,
		format = '0x%04x'
	},
	u8 = {
		pack = 'I1',
		size = 1,
		format = '0x%02x',
	},
	i64 = {
		pack = 'i8',
		size = 8,
		format = '0x%016x'
	},
	i32 = {
		pack = 'i4',
		size = 4,
		format = '0x%08x',
		mask = 0xffffffff
	},
	i16 = {
		pack = 'i2',
		size = 2,
		format = '0x%04x',
		mask = 0xffff
	},
	i8 = {
		pack = 'i1',
		size = 1,
		format = '0x%02x',
		mask = 0xff
	},
	f4 = {
		pack = 'f',
		size = 4,
		format = '%0.2f',
	},
	f8 = {
		pack = 'd',
		size = 8,
		format = '%0.2f',
	},
	ba = {
		size = 1,
		array_cls = byte_array_field_def,
	},
	zs = {
		size = 1,
		array_cls = zeroterm_byte_array_field_def
	}
}




local field_stub = class()
function field_stub:_init(size)
	self.size = size
end
function field_stub:load(dst,src)
end
function field_stub:read(dst,d,o)
	return o + self.size
end
function field_stub:write(dst)
	table.insert(dst,string.rep('\0',self.size))
end
function field_stub:write_data(dst,val)
	table.insert(dst,string.rep('\0',self.size))
end
function field_stub:dump()
end

local simple_field_def = class(field_def)

function simple_field_def:dump_data(data,out,o)
	out(o..self:format(data))
end

function simple_field_def:dump(data,out,o)
	out(o..self.name,self:format(data[self.name]))
end



local function make_simple_def(def,pack)
	local cls = class(simple_field_def)
	cls.size = def.size
	function cls:read_data(data,offset)
		local res = data.unpack(pack,data,offset+1)
		return res, offset + self.size
	end
	function cls:write_data(dst,val)
		table.insert(dst,string.pack(pack,val))
	end
	local format = def.format
	if def.mask then
		local mask = def.mask
		function cls:format(val)
			return string.format(format,val & mask)
		end
	else
		function cls:format(val)
			return string.format(format,val)
		end
	end

	cls.arrays_cls = def.arrays or {}
	return cls
end

for _,v in pairs(data_types) do
	if v.array_cls then
		v.is_array = true
	else
		if v.size > 1 then
			v.cls_le = make_simple_def(v,'<' .. v.pack)
			v.cls_be = make_simple_def(v,'>' .. v.pack)
		else
			local cls = make_simple_def(v,v.pack)
			v.cls_le = cls
			v.cls_be = cls
		end
		v.stub = field_stub.new(v.size)
	end
end

local struct_def = class(nil,'struct_def')
local field_struct_def = class(struct_def)
local field_struct_def_wrap = class(field_def)

function struct_def:_init(fields,endian)
	self._fields = {}
	self._endian = fields.endian or endian or 'le'
	local cls_name = 'cls_' .. self._endian
	self.size = 0
	for _,f in ipairs(fields) do
		local ftype = f[1]
		local fname = f[2]
		local fd 
		local ftypetype = type(ftype)
		local array_created 
		if ftypetype == 'table' then
			if not ftype.is_a then
				fd = field_struct_def.new(ftype,fname,f,endian)
			elseif ftype.is_a[struct_def] then
				fd = field_struct_def_wrap.new(ftype,fname)
			elseif not ftype.is_a[struct_def] then
				error('unexpected field type :' .. tostring(ftype))
			end
		elseif ftypetype == 'string' then
			local type_def = data_types[ftype] or error('unsupported field type: ' .. tostring(ftype))
			if not fname then
				if f[3] then
					fd = field_stub.new(type_def.size * f[3])
					array_created = true
				else
					fd = type_def.stub
				end
			elseif type_def.is_array then
				if not f[3] then
					error('need length for type: ' .. tostring(ftype))
				end
				fd = type_def.array_cls.new(fname,f[3],f)
				array_created = true
			else
				fd = type_def[cls_name].new(fname,f)
			end
		else
			error('unexpected field type:' .. tostring(ftypetype) .. '/' .. tostring(ftype))
		end

		if f[3] and not array_created then
			fd = array_field_def.new(fd,f[3])
		end
		self.size = self.size + fd.size
		table.insert(self._fields,fd)
	end
end

function struct_def:get_field(name)
	for _,v in ipairs(self._fields) do
		if v.name == name then
			return v
		end
	end
end

function struct_def:load(dst,src)
	for _,f in ipairs(self._fields) do
		f:load(dst,src)
	end
end

function struct_def:dump_fields(data,out,o)
	assert(out)
	for _,f in ipairs(self._fields) do
		f:dump(data,out,o)
	end
end

function struct_def:tostring_fields(data,o)
	local r = {}
	for _,f in ipairs(self._fields) do
		local s = f:tostring(data)
		if s then
			table.insert(r,s)
		end
	end
	return table.concat(r,',' .. (o or ''))
end

function struct_def:tostring_fields(data,o)
	local res = {}
	for _,v in ipairs(self._fields) do
		table.insert(res, v:tostring(data,o))
	end
	return table.concat(res,',')
end

function struct_def:read_fields(dst,d,o)
	assert(o)
	for _,f in ipairs(self._fields) do
		o = f:read(dst,d,o)
	end
	return o
end

function struct_def:write_fields(dst,d)
	for _,f in ipairs(self._fields) do
		f:write(dst,d)
	end
end


function struct_def:compile()
end

function field_struct_def:_init(fields,name,fdef,endian)
	field_struct_def.baseclass._init(self,fields,endian)
	self.name = name
end

function field_struct_def:load(dst,src)
	local ssrc = src[self.name] or error('need struct field: ' .. self.name)
	dst[self.name] = ssrc
end

function field_struct_def:dump(data,out,o)
	out(o .. self.name,'>')
	o = o .. '\t'
	field_struct_def.baseclass.dump_fields(self,data[self.name],out,o)
end

function field_struct_def:dump_data(data,out,o)
	o = o .. '\t'
	field_struct_def.baseclass.dump_fields(self,data,out,o)
end

function field_struct_def:read(dst,d,o)
	local ddst = {}
	o = field_struct_def.baseclass.read_fields(self,ddst,d,o)
	dst[self.name] = ddst
	return o
end

function field_struct_def:write(dst,d,o)
	return field_struct_def.baseclass.write_fields(self,dst,d[self.name],o)
end

function field_struct_def:write_data(dst,d,o)
	field_struct_def.baseclass.write_fields(self,dst,d,o)
end

function field_struct_def:read_data(d,o)
	assert(o)
	local r = {}
	o = field_struct_def.baseclass.read_fields(self,r,d,o)
	return r,o
end

function field_struct_def_wrap:_init(def,name)
	field_struct_def_wrap.baseclass._init(self,name)
	self._wrap = def
	self.size = self._wrap.size
end

function field_struct_def_wrap:load(dst,src)
	local dsrc = src[self.name] or error('need struct field: ' .. self.name)
	dst[self.name] = dsrc
end

function field_struct_def_wrap:dump(data,out,o)
	out(o .. self.name,'>')
	o = o .. '\t'
	self._wrap:dump_fields(data[self.name],out,o)
end
function field_struct_def_wrap:dump_data(data,out,o)
	o = o .. '\t'
	self._wrap:dump_fields(data,out,o)
end


function field_struct_def_wrap:write_data(dst,val)
	self._wrap:write_fields(dst,val)
end

function field_struct_def_wrap:read_data(d,o)
	local r = {}
	assert(o)
	o = self._wrap:read_fields(r,d,o)
	return r,o
end



local struct = class(nil,'struct')

function struct:_init(s,d)
	if s.is_a then
		assert(s.is_a[struct_def],'invalid struct def: ' .. tostring(s))
		self._def = s
	else
		self._def = struct_def.new(s)
	end
	if d then
		self:load(d)
	end
end

function struct:load(d)
	self._def:load(self,d)
end


function struct:dump( out, o )
	local prnt = out or print
	local p = o or ''
	self._def:dump_fields(self,prnt,p)
end


function struct:read( d,offset )
	local o = offset or 0
	o = self._def:read_fields( self, d , o )
	return o
end

function struct:build( )
	local r = {}
	self._def:write_fields(r,self)
	return table.concat(r,'')
end

function _M.sizeof( s )
	local r = 0
	if s.size then
		return s.size
	end
	for _,v in ipairs( s ) do
		local fs 
		if type(v[1]) == 'table' then
			fs = _M.sizeof(v[1])
		else
			local d = data_types[v[1]] or error('undefined type:' .. tostring(v[1]))
			fs = d.size
		end
		r = r + fs * (v[3] or 1)
	end
	return r
end

function _M.offsetof( s , f )
	local r = 0
	for _,v in ipairs(s) do
		if f == v[2] then
			return r
		end
		local fs
		if type(v[1]) == 'table' then
			fs = _M.sizeof(v[1])
		else
			local d = data_types[v[1]] or error('undefined type:' .. tostring(v[1]))
			fs = d.size
		end
		r = r + s * (v[3] or 1)
	end
	return nil
end

function _M.build( struct_def, data )
	local r = struct.new(struct_def,data)
	return r:build()
end

function _M.read( d, struct_def, offset )
	local r = struct.new(struct_def)
	local o = r:read(d,offset)
	return r,o
end


function _M.define( s )
	return struct_def.new(s)
end

function _M.compile( s )
	local res = struct_def.new(s)
	res:compile()
	return res
end

return _M