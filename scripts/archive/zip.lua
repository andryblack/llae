local class = require 'llae.class'
local fs = require 'llae.fs'
local struct = require 'llae.struct'
local log = require 'llae.log'
local compression = require 'llae.compression'
local path = require 'llae.path'

local zip = class(nil,'zip')

local BUFREADCOMMENT = 0x400
local Z_DEFLATED   = 8

local SIZECENTRALDIRITEM = (0x2e)
local SIZEZIPLOCALHEADER = (0x1e)

local EOCD_t = {
	{'u32','sig'}, 						-- End of central directory signature = 0x06054b50
	{'u16','number_disk'},  			-- Number of this disk
	{'u16','number_disk_with_CD'},		-- Disk where central directory starts 
	{'u16','disk_records'},				-- Number of central directory records on this disk
	{'u16','total_records'},			-- Total number of central directory records
	{'u32','size_central_dir'}, 		-- Size of central directory (bytes)
	{'u32','offset_central_dir'}, 		-- Offset of start of central directory, relative to start of archive
	{'u16','comment_length'},
}
local EOCD_size = struct.sizeof(EOCD_t)

local CD_t = {
	{'u32','sig'},						-- Central directory file header signature = 0x02014b50
	{'u16','version_made'},				-- Version made by
	{'u16','version_extract'},			-- Version needed to extract (minimum)
	{'u16','gp_flags'},					-- General purpose bit flag
	{'u16','compression'},				-- Compression method
	{'u16','mod_time'},					-- File last modification time
	{'u16','mod_date'},					-- File last modification date
	{'u32','crc'},						-- CRC-32 of uncompressed data
	{'u32','compressed_size'},			-- Compressed size (or 0xffffffff for ZIP64)
	{'u32','uncompressed_size'},		-- Uncompressed size (or 0xffffffff for ZIP64)
	{'u16','file_name_len'},			-- File name length (n)
	{'u16','extra_field_len'},			-- Extra field length (m)
	{'u16','file_comment_len'},			-- File comment length (k)
	{'u16','start_disk'},				-- Disk number where file starts
	{'u16','internal_attr'},			-- Internal file attributes
	{'u32','external_attr'},			-- External file attributes
	{'u32','local_offset'},				-- Relative offset of local file header. This is the number of bytes between the start of the first disk on which the file occurs, and the start of the local file header. This allows software reading the central directory to locate the position of the file inside the ZIP file.
}
local CD_size = struct.sizeof(CD_t)
assert(CD_size == SIZECENTRALDIRITEM)

local LH_t = {
	{'u32','sig'},						-- Local file header signature = 0x04034b50 (read as a little-endian number)
	{'u16','version_extract'},			-- Version needed to extract (minimum)
	{'u16','gp_flags'},					-- General purpose bit flag
	{'u16','compression'},				-- Compression method; e.g. 0x0000 = none, 0x0800 = DEFLATE,
	{'u16','mod_time'},					-- File last modification time
	{'u16','mod_date'},					-- File last modification date
	{'u32','crc'},						-- CRC-32 of uncompressed data
	{'u32','compressed_size'},			-- Compressed size (or 0xffffffff for ZIP64)
	{'u32','uncompressed_size'},		-- Uncompressed size (or 0xffffffff for ZIP64)
	{'u16','file_name_len'},			-- File name length (n)
	{'u16','extra_field_len'},			-- Extra field length (m)
}
local LH_size = struct.sizeof(LH_t)
assert(LH_size == SIZEZIPLOCALHEADER)

local file_info = class(nil,'zip file')

function file_info:_init( cd, lh , ci)
	self._cd = cd
	self._lh = lh
	self._ci = ci
	-- body
end

function file_info:get_name(  )
	return self._cd.name
end

function file_info:get_cd_size(  )
	return CD_size + self._cd.file_name_len + self._cd.extra_field_len + self._cd.file_comment_len
end

function file_info:get_data_offset(  )
	return self._cd.local_offset + LH_size + self._lh.file_name_len + self._lh.extra_field_len
end

function file_info:get_compressed_size(  )
	return self._cd.compressed_size
end

function file_info:get_uncompressed_size(  )
	return self._cd.uncompressed_size
end

function file_info:match( filename )
	if self._ci then
		return string.upper(self._cd.name) == string.upper(filename)
	end
	return self._cd.name == filename
end

function file_info:is_deflated(  )
	return self._cd.compression == Z_DEFLATED
end

function zip:_init( reader , size , options)
	self._reader = reader
	self._size = size
	self._files_list = {}
	self._case_insensitive = options and options.caseinsensitive
end

function zip:_search_central_dir(  )
	local uMaxBack = math.min(0xffff,self._size)
	local uBackRead = 4
	while uBackRead < uMaxBack do
		if (uBackRead+BUFREADCOMMENT>uMaxBack) then
            uBackRead = uMaxBack;
        else
            uBackRead=uBackRead + BUFREADCOMMENT
        end
        local uReadPos = self._size-uBackRead
        local uReadSize = math.min(BUFREADCOMMENT+4,self._size-uReadPos) 
        self._reader:seek( uReadPos )
        local ch,err = self._reader:read(uReadSize)
        if not ch then
        	return ch,err
        end

        for i=uReadSize-3,0,-1 do
        	local a,b,c,d = ch:byte(i+1,i+4)
        	if a == 0x50 and b == 0x4b and c == 0x05 and d == 0x06 then
        		return uReadPos+i
        	end
        end

	end
end

function zip:_get_file_info(  )
	self._reader:seek(self._pos_in_central_dir+self._byte_before_the_zipfile)
	local cd_data,err = self._reader:read(CD_size)
	if not cd_data then
		return cd_data,err
	end
	if #cd_data ~= CD_size then
		return nil,'file end'
	end
	local cd,err = struct.read(tostring(cd_data),CD_t)
	if not cd then
		return cd,err
	end
	if cd.sig ~= 0x02014b50 then
		return nil,'invalid file'
	end
	--cd:dump(nil,log.info)
	local name = tostring(self._reader:read(cd.file_name_len))
	local extra = cd.extra_field_len ~=0 and self._reader:read(cd.extra_field_len)
	local comment = cd.file_comment_len ~= 0 and self._reader:read(cd.file_comment_len)
	cd.name = name
	cd.extra = extra
	cd.comment = comment
	log.debug('file:',name)

	self._reader:seek(cd.local_offset+self._byte_before_the_zipfile)
	local lh_data,err = self._reader:read(LH_size)
	if not lh_data then
		return lh_data,err
	end
	if #lh_data ~= LH_size then
		return nil,'file end'
	end
	local lh,err = struct.read(tostring(lh_data),LH_t)
	if lh.sig ~= 0x04034b50 then
		return nil,'invalid local file'
	end

	return file_info.new(cd,lh,self._case_insensitive)
end

function zip:_goto_first_file(  )
	self._pos_in_central_dir = self._offset_central_dir
	self._current_file = self:_get_file_info()
	return self._current_file
end

function zip:_goto_next_file(  )
	self._pos_in_central_dir = self._pos_in_central_dir + self._current_file:get_cd_size()
	self._current_file =  self:_get_file_info()
	return self._current_file
end

function zip:scan( handler )
	local central_pos = self:_search_central_dir()
	if not central_pos then
		return nil,'not found central dir'
	end
	self._reader:seek(central_pos)
	local eocd_data = tostring(self._reader:read(EOCD_size))
	self._eocd = struct.read(eocd_data,EOCD_t)
	if self._eocd.disk_records ~= self._eocd.total_records then
		return nil,'invalid ZIP'
	end
	if central_pos<(self._eocd.offset_central_dir+self._eocd.size_central_dir) then
		return nil,'invalid ZIP'
	end
	self._central_pos = central_pos
	self._offset_central_dir = self._eocd.offset_central_dir
	self._size_central_dir = self._eocd.size_central_dir
	self._byte_before_the_zipfile = central_pos -
                            (self._offset_central_dir+self._size_central_dir);
    local info,err = self:_goto_first_file()
    if not info then
    	return info,err
    end
	table.insert(self._files_list,info)
    if handler then
		handler(info:get_name())
	end
    while true do
    	local info,err = self:_goto_next_file()
    	if info then
    		table.insert(self._files_list,info)
    		if handler then
    			handler(info:get_name())
    		end
    	else
    		break
    	end
    end
    return true
end

local zip_file = class(nil, 'zip file')
function zip_file:_init( reader, info , goffset )
	self._reader = reader
	self._info = info
	self._byte_before_the_zipfile = goffset
	self._pos_in_zipfile = info:get_data_offset()
	self._uncompressed_available = info:get_uncompressed_size()
	self._compressed_available = info:get_compressed_size()
	if info:is_deflated() then
		self._deflate = compression.new_inflate_read(true)
	end
end

function zip_file:_raw_read( size )
	local s = math.min(size,self._compressed_available)
	self._reader:seek(self._pos_in_zipfile + self._byte_before_the_zipfile)
	local ch,err = self._reader:read(s)
	if not ch then
		return ch,err or 'eof'
	end
	if #ch ~= s then
		return nil,'failed read file'
	end
	self._pos_in_zipfile = self._pos_in_zipfile + s
	self._compressed_available = self._compressed_available - s
	return ch
end

function zip_file:read( size )
	local s = math.min(size,self._uncompressed_available)
	local r = {}
	--log.info('start read',size,s,self._uncompressed_available,self._compressed_available )
	while s > 0 do
		if self._data then
			local ss = math.min(s,#self._data)
			table.insert(r,self._data:sub(1,ss))
			if ss ~= #self._data then
				self._data = self._data:sub(ss+1)
			else
				self._data = nil
			end
			s = s - ss
			self._uncompressed_available = self._uncompressed_available - ss
			--log.info('flushed',ss,'wait',s)
		else
			if self._deflate then
				if self._compressed_available > 0 then
					local ch,err = self:_raw_read(1024*4)
					if not ch then
						return ch,err
					end
					--log.info('write to compressor',#ch,self._compressed_available)
					local res,err = self._deflate:write(ch)
					if not res then
						--log.info('write result',res,err)
						return res,err
					end
					--log.info('write finished',self._compressed_available )
					if self._compressed_available == 0 then
						local res,err = self._deflate:finish()
						if not res then
							--log.info('finish result',res,err)
							return res,err
						end
						--log.info('finish finished')
					end
				end
				--log.info('read compressor')
				local ch,er = self._deflate:read(false)
				--log.info('readed',ch and #ch or 'nil',er)
				if ch then
					self._data = ch
				elseif err then
					return nil,err
				else
					
				end
			else
				local ch,err = self:_raw_read(1024*4)
				if not ch then
					return ch,err
				end
				self._data = ch
			end
		end
	end
	if not next(r) then
		return nil
	end
	--log.info('read finished')
	return table.concat(r)
end

function zip_file:close( check )
	if check then
		assert(self._compressed_available == 0)
		assert(self._uncompressed_available == 0)
	end
end

function zip:_read_file( info )
	local f = zip_file.new(self._reader,info,self._byte_before_the_zipfile)
	local d = {}
	while true do
		local ch,er = f:read(1024*4)
		if not ch then
			if err then
				return ch,err
			end
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d)
end

function zip:close(  )
	self._reader:close()
end

function zip:read_file( filename )
	for _,v in ipairs(self._files_list) do
		if v:match(filename) then
			return self:_read_file(v)
		end
	end
	return nil,'not found'
end

function zip:open_file( filename )
	for _,info in ipairs(self._files_list) do
		if info:match(filename) then
			return zip_file.new(self._reader,info,self._byte_before_the_zipfile)
		end
	end
end

function zip.open( filename , options )
	local st = assert(fs.stat(filename))
	local f = assert(fs.open(filename,fs.O_RDONLY))
	local z = zip.new(f,st.size,options)
	local files = {}
	assert(z:scan())
	return z
end

function zip.unpack_zip( filename , dst )
	local st = assert(fs.stat(filename))
	local f = assert(fs.open(filename,fs.O_RDONLY))
	local z = zip.new(f,st.size)
	local files = {}
	assert(z:scan( function(fn)
		if fn:sub(-1) ~= '/' then
			--log.info('found file',fn)
			table.insert(files,fn)
		end
	end))
	for _,v in ipairs(files) do

		local cf = assert(z:open_file(v))
		local dest_fn = path.join(dst,v)
		log.debug('write file',dest_fn,type(d))
		fs.mkdir_r(path.dirname(dest_fn))
		fs.unlink(dest_fn)
		local f = fs.open(dest_fn,fs.O_WRONLY|fs.O_CREAT)
		while true do
			local d,e = cf:read(1024*4)
			if not d then
				break
			end
			f:write(d)
		end
		cf:close(true)
		f:close()
	end
	z:close()
end

return zip