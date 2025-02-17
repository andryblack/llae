local class = require 'llae.class'
local struct = require 'llae.struct'
local log = require 'llae.log'
local fs = require 'llae.fs'
local path = require 'llae.path'

local untar = class(nil,'untar')

local posix_header_t = {
	{'zs','name',100},
	{'zs','mode',8},
	{'zs','uid',8},--			/* 108 */
	{'zs','gid',8},--			/* 116 */
	{'zs','size',12},--		/* 124 */
	{'zs','mtime',12},--		/* 136 */
	{'u8','chksum',8},--		/* 148 */
	{'u8','typeflag'},--		/* 156 */
	{'zs','linkname',100},--		/* 157 */
	{'u8','magic',6},--		/* 257 */
	{'u8','version',2},--		/* 263 */
	{'zs','uname',32},--		/* 265 */
	{'zs','gname',32},--		/* 297 */
	{'u8','devmajor',8},--		/* 329 */
	{'u8','devminor',8},--		/* 337 */
	{'zs','prefix',155}--		/* 345 */
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


function untar:get_path( fn )
	return fn
end

function untar:prepare_file(  )
	if #self._header.name == 0 and
		self._header.typeflag == 0 then
		self._state = 'align'
		self._next = 'end'
		self._file = nil
		return
	end
	local name = self._header.name

	local prefix = self._header.prefix
	if prefix and #prefix > 0 then
		name = prefix .. '/' .. name
	end
	local prev_file = self._file
	self._file = {
		name = name,
		size = tonumber(self._header.size,8),
		mtime = tonumber(self._header.mtime),
		readed = 0
	}
	if prev_file and prev_file.long_name then
		self._file.name = prev_file.long_name
		--log.debug('long name',self._file.name,#prev_file.long_name,name)
	end
	if self._header.typeflag == string.byte('5') then
		--log.debug('found dir',self._file.name)
		assert(self._file.size == 0)
		local fn = self:get_path(self._file.name)
		if fn then 
			fs.mkdir(fn)
		end
		self._state = 'align'
		self._next = 'header'
	elseif self._header.typeflag == string.byte('L') then
		--log.debug('start long',self._file.name,self._file.size)
		self._next = 'long_name'
		self._state = 'align'
		self._file.long_name = ''
	else
		--log.debug('start file',self._file.name,self._file.size)
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

function untar:process_long_name( data )
	self._data = self._data .. data
	local cnt = math.min(#self._data,(self._file.size-self._file.readed))
	if cnt ~= 0 then
		local d = string.sub(self._data,1,cnt)
		self._data = string.sub(self._data,cnt+1)
		self._file.readed = self._file.readed + cnt
		self._processed = self._processed + cnt
		self._file.long_name = self._file.long_name .. d
	end
	if self._file.readed >= self._file.size then
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

local function do_unpack(f,u, dir , strip)
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
			--log.debug(sfn,'->',fn)
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

local raw_read = class()

function raw_read:_init()
	self._data = {}
end
function raw_read:finish()
	self._finished = true
end
function raw_read:write(ch)
	table.insert(self._data,ch)
end
function raw_read:read()
	local ch = table.remove(self._data,1)
	return ch,nil
end

function untar.unpack_tar( fn, dir , strip )
	local f = assert(fs.open(fn,fs.O_RDONLY))
	local u = raw_read.new()
	return do_unpack(f,u,dir,strip)
end

function untar.unpack_tgz( fn, dir , strip )
	local f = assert(fs.open(fn,fs.O_RDONLY))
	local u = (require 'archive').new_gunzip_read()
	return do_unpack(f,u,dir,strip)
end



function untar.unpack_tbz2( fn, dir , strip )
	local f = assert(fs.open(fn,fs.O_RDONLY))
	local u = (require 'archive.bzip2').new_bz_read()
	return do_unpack(f,u,dir,strip)
end

return untar