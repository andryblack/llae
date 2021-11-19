local class = require 'llae.class'
local struct = require 'llae.struct'
local log = require 'llae.log'
local fs = require 'llae.fs'
local path = require 'llae.path'

local untar = class(nil,'untar')

local posix_header_t = {
	{'u8','name',100,zeroterm=true},
	{'u8','mode',8,zeroterm=true},
	{'u8','uid',8,zeroterm=true},--			/* 108 */
	{'u8','gid',8,zeroterm=true},--			/* 116 */
	{'u8','size',12,zeroterm=true},--		/* 124 */
	{'u8','mtime',12,zeroterm=true},--		/* 136 */
	{'u8','chksum',8},--		/* 148 */
	{'u8','typeflag'},--		/* 156 */
	{'u8','linkname',100,zeroterm=true},--		/* 157 */
	{'u8','magic',6},--		/* 257 */
	{'u8','version',2},--		/* 263 */
	{'u8','uname',32,zeroterm=true},--		/* 265 */
	{'u8','gname',32,zeroterm=true},--		/* 297 */
	{'u8','devmajor',8},--		/* 329 */
	{'u8','devminor',8},--		/* 337 */
	{'u8','prefix',155}--		/* 345 */
}
local posix_header_size = struct.sizeof(posix_header_t)
assert( posix_header_size == 500 )

function untar:_init( )
	--self._file = file
	self._state = 'header'
	self._data = ''
	self._processed = 0
end

function untar:process_header( data )
	self._data = self._data .. data
	if #self._data >= posix_header_size then
		self._header = struct.read(self._data,posix_header_t)
		self._data = string.sub(self._data,posix_header_size+1)
		self._processed = self._processed + posix_header_size
		--log.debug('found header')
		--self._header:dump(nil,log.debug)
		self._state = 'file'
		self:prepare_file()
		return true
	end
	return false
end

local function parse_name(data)
	local r = {}
	for _,v in ipairs(data) do
		if v == 0 then
			break
		else
			table.insert(r,string.char(v))
		end
	end
	return table.concat(r,'')
end

function untar:get_path( fn )
	return fn
end

function untar:prepare_file(  )
	if self._header.name[1] == 0 and
		self._header.typeflag == 0 then
		self._state = 'align'
		self._next = 'end'
		self._file = nil
		return
	end
	local name = parse_name(self._header.name)
	local prefix = parse_name(self._header.prefix)
	if prefix and #prefix > 0 then
		name = prefix .. '/' .. name
	end
	self._file = {
		name = name,
		size = tonumber(parse_name(self._header.size),8),
		mtime = tonumber(parse_name(self._header.mtime)),
		readed = 0
	}
	if self._header.typeflag == string.byte('5') then
		--log.debug('found dir',self._file.name)
		assert(self._file.size == 0)
		local fn = self:get_path(self._file.name)
		if fn then 
			fs.mkdir(fn)
		end
		self._state = 'align'
		self._next = 'header'
	else
		log.debug('start file',self._file.name,self._file.size)
		self._next = 'file'
		self._state = 'align'
		local fn = self:get_path(self._file.name)
		if fn then
			fs.mkdir_r(path.dirname(fn))
			fs.unlink(fn)
			self._file.f = assert(fs.open(fn,fs.O_WRONLY|fs.O_CREAT))
		else
			-- skip file
		end
	end
end

function untar:process_file( data )
	self._data = self._data .. data
	local cnt = math.min(#self._data,(self._file.size-self._file.readed))
	if cnt ~= 0 then
		local d = string.sub(self._data,1,cnt)
		self._data = string.sub(self._data,cnt+1)
		self._file.readed = self._file.readed + cnt
		self._processed = self._processed + cnt
		if self._file.f then
			assert(self._file.f:write(d))
		end
	end
	if self._file.readed >= self._file.size then
		--log.debug('end file',self._file.name,self._file.readed)
		if self._file.f then
			self._file.f:close()
		end
		self._state = 'align'
		self._next = 'header'
		return true
	end
	return false
end

function untar:process_align( data )
	self._data = self._data .. data
	local unaligned = self._processed % 512
	if unaligned ~= 0 then
		local skip = 512 - unaligned
		if #self._data < skip then
			return false
		end
		--log.debug('skip',skip)
		self._data = string.sub(self._data,skip+1)
		self._processed = self._processed + skip
	end
	--log.debug('aligned')
	self._state = self._next
	return true
end

function untar:process_end(  )
	return false
end

function untar:write( data )
	while self['process_'..self._state](self,data) do
		data = ''
	end
end

function untar.unpack_tgz( fn, dir , strip )
	local f = assert(fs.open(fn,fs.O_RDONLY))
	local u = (require 'archive').new_gunzip_read()
	local t = untar.new()
	if dir or strip then
		local path = require 'llae.path'
		function t:get_path(fn)
			local sfn = fn
			if strip then
				fn = path.remove_leading_dirs(fn,strip)
			end
			if not fn then
				return nil
			end
			if dir then
				fn = path.join(dir,fn)
			end
			log.debug(sfn,'->',fn)
			return fn
		end
	end
	while true do
		if f then
			local ch,e = f:read(1024*32)
			if ch then
				u:write(ch)
			elseif e then
				error(e)
			else
				u:finish()
				f:close()
				f = nil
			end
		end
		local ch,e = u:read(not f)
		if ch then
			t:write(ch)
		elseif e then
			error(e)
		else
			break
		end
	end
end

return untar