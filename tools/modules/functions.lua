local async = require 'llae.async'
local log = require 'llae.log'
local fs = require 'llae.fs'
local path = require 'llae.path'
local crypto = require 'llae.crypto'
local untar = require 'archive.tar'
local unzip = require 'archive.zip'
local http = require 'net.http'
local os = require 'llae.os'
local netutils = require 'net.utils'

local uv = require 'uv'

local m = {}

local function redirect_pipe(pipe,file)
	async.run(function()
		local d = ''
		while true do
			local ch,err = pipe:read()
			if not ch then
				if d and #d > 0 then	
					file:write(d)
				end
				if err then
					log.error('pipe read failed:',err)
					file:write('read failed:',err)
					--error(err)
					break
				else
					break
				end
			else
				d = d .. ch
				local el = d:find('\n',1,true)
				if el then
					log.debug(d:sub(1,el-1))
					d = d:sub(el+1)
				end
				file:write(ch)
			end
		end
	end)
end

local function exec_cmd(cmd,args,logfile,cwd)
	log.debug(cmd,table.concat(args,' '))
	logfile:write('# at ' .. (cwd or fs.cwd()) .. '\n')
	logfile:write('$ '..cmd..' ' .. table.concat(args,' ') .. '\n')
	local rpipe = uv.pipe.new(1)
	local epipe = uv.pipe.new(1)
	local p = assert(uv.process.spawn{
		file = cmd,
		args = args,
		cwd = cwd,
		streams = {
			{uv.process.IGNORE},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,rpipe},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,epipe}
		}
	},'failed spawn ' .. tostring(cmd))
	redirect_pipe(rpipe,logfile)
	redirect_pipe(epipe,logfile)
	
	local code,sig = p:wait_exit()
	rpipe:close()
	epipe:close()
	if code ~= 0 or sig ~= 0 then
		error(string.format(cmd .. ' code:%d sig:%d',code,sig))
	end
end

local function exec_git(args,logfile)
	return exec_cmd('git',args,logfile)
end

function m:download_git(url,config)
	local dst = path.join(self.location,config.dir or 'src')
	log.info('download_git',self.name,url,dst)
	local tag = config.tag or config.branch or 'master'
	local logfilename = path.join(self.location,'update_git_log.txt')
	fs.unlink(logfilename)
	local logfile = assert(fs.open_write(logfilename))

	if fs.isdir(dst) and fs.isdir(path.join(dst,'.git')) then
		exec_git({'-C',dst,'reset','--hard'},logfile)
		
			
		if config.tag then
			exec_git({'-C',dst,'fetch','origin','tags/' .. config.tag},logfile)
			exec_git({'-C',dst,'reset','--hard',config.tag},logfile)
		else
			exec_git({'-C',dst,'fetch','origin',tag},logfile)
			exec_git({'-C',dst,'reset','--hard','origin/' .. tag},logfile)
		end
		return
	end
	fs.rmdir_r(dst)
	exec_git({'clone','--depth','1','--branch',tag,'--single-branch',url,dst},logfile)
	logfile:close()
end




function m:download(url,file,hash)
	log.info('download',self.name,url)
	local dst = path.join(self._project:get_dl_dir(),file)
	local htype = 'MD5'
	local hval = hash
	if hash then
		local sep = string.find(hash,':',1,true)
		if sep then
			htype = string.upper(hash:sub(1,sep-1))
			hval = hash:sub(sep+1)
		end
	end
	local res,err = netutils.download_file(url,dst,{
		hash=hval,
		hash_type=htype,
		log=log,
		progress_func=log.progress
	})
	if not res then
		error(err)
	end
	return res
end

local function _local(self,fn)
	if path.isabsolute(fn) then
		return fn
	end
	return path.join(self.location,fn)
end

function m:download_file(url,file)
	local dst = _local(self,file)
	fs.mkdir_r(path.dirname(dst))
	log.debug('download file',url,file)
	local res,err = netutils.download_file(url,dst,{
		log=log,progress=log.progress()
	})
	if not res then
		error(err)
	end
	return res
end

function m:unpack_tgz( file , todir , strip)
	local src = path.join(self._project:get_dl_dir(),file)
	local dst = todir and _local(self,todir) or self.location
	log.info('unpack',file)
	untar.unpack_tgz(src,dst,strip)
end

function m:unpack_tbz2( file , todir , strip)
	local src = path.join(self._project:get_dl_dir(),file)
	local dst = todir and _local(self,todir) or self.location
	log.info('unpack',file)
	untar.unpack_tbz2(src,dst,strip)
end

function m:unpack_zip( file , todir )
	local src = path.join(self._project:get_dl_dir(),file)
	local dst = todir and _local(self,todir) or self.location
	log.info('unpack',file)
	unzip.unpack_zip(src,dst)
end

function m:unpack_txz( file , todir , strip)
	local src = path.join(self._project:get_dl_dir(),file)
	local dst = todir and _local(self,todir) or self.location
	log.info('unpack',file)
	untar.unpack_txz(src,dst,strip)
end



function m:get_self_exe()
	return fs.exepath()
end

local function get_exename(root,bin)
	if path.isabsolute(bin) then
		return bin
	end
	local exename = path.join(root,bin)
	if fs.isfile(exename) then
		return exename
	end
	return fs.find_exe(bin)
end

function m:exec(config)
	local bin = config.bin or error('need bin')
	local args = config.args or {}
	local name = config.name
	local exename = get_exename(self.root,bin)
	local logfilename = path.join(self.location, (name or ('exec_'..bin) ) .. '_log.txt')
	log.info('cmd:',exename,table.concat( args, ' ' ),'>',logfilename)
	local cwd = config.cwd or path.getabsolute(self.location)
	log.info('at', cwd)
	fs.unlink(logfilename)
	local logfile = assert(fs.open_write(logfilename))
	local env = nil
	if config.env then
		env = os.getallenv()
		for k,v in pairs(config.env) do
			env[k]=v
		end
	end
	local rpipe = uv.pipe.new(1)
	local epipe = uv.pipe.new(1)
	local p = assert(uv.process.spawn{
		file = exename,
		args = args,
		env = env,
		cwd = cwd,
		streams = {
			{uv.process.IGNORE},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,rpipe},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,epipe}
		}
	})
	redirect_pipe(rpipe,logfile)
	redirect_pipe(epipe,logfile)
	local err
	local code,sig = p:wait_exit()
	if code ~= 0 or sig ~= 0 then
		err = string.format('process code:%d sig:%d',code,sig)
	end
	logfile:close()
	if err and self._project:get_cmdargs().development then
		log.error('Failed:',err)
		log.info('cmd log:',fs.load_file(logfilename))
	end
	rpipe:close()
	epipe:close()
	return not err,err
end

function m:exec_res(config)
	local bin = config.bin or error('need bin')
	local args = config.args or {}
	local cwd = config.cwd or path.getabsolute(self.location)
	local env = nil
	if config.env then
		env = os.getallenv()
		for k,v in pairs(config.env) do
			env[k]=v
		end
	end

	local exename = get_exename(self.root,bin)
	local rpipe = uv.pipe.new(1)
	local epipe = uv.pipe.new(1)
	local p = assert(uv.process.spawn{
		file = exename,
		args = args,
		cwd = cwd,
		streams = {
			{uv.process.IGNORE},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,rpipe},
			{uv.process.CREATE_PIPE|uv.process.WRITABLE_PIPE,epipe}
		}
	})
	local result = {}
	local function read_pipe(pipe)
		async.run(function()
			while true do
				local ch,err = pipe:read()
				if not ch then
					if err then
						error(err)
					else
						break
					end
				else
					table.insert(result,tostring(ch))
				end
			end
		end)
	end
	read_pipe(rpipe)
	read_pipe(epipe)

	
	local code,sig = p:wait_exit()

	rpipe:close()
	epipe:close()
	
	if code ~= 0 or sig ~= 0 then
		log.error('exec_res failed:',exename,table.concat( args, ' ' ))
		log.error(table.concat(result,'\n'))
		error(string.format('process code:%d sig:%d',code,sig))
	end
	
	result = table.concat(result,'')
	log.info('cmd:',exename,table.concat( args, ' ' ),'>',result)
	return result
end


function m:get_absolute_location(...)
	return path.getabsolute(path.join(self.location,...))
end

function m:install_bin( fn )
	local src = path.join(self.location,fn)
	local dst = path.join(self.root,'bin',path.basename(fn))
	log.info('install',src,'->',dst)
	assert(fs.copyfile(src,dst))
end

function m:install_files( files )
	for to,from in pairs(files) do
		local src = _local(self,from)
		local dst = path.join(self.root,to)
		fs.mkdir(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.mkdir_r(path.dirname(dst))
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:write_file( filename , content)
	if not path.isabsolute(filename) then
		filename = path.join(self.root,filename)
	end
	fs.mkdir_r(path.dirname(filename))
	fs.unlink(filename)
	fs.write_file(filename,content)
end

function m:read_file( filename )
	local src = _local(self,filename)
	local content = fs.load_file(src)
	return content
end

function m:read_dl_file( filename )
	local src = path.join(self._project:get_dl_dir(),filename)
	local content = fs.load_file(src)
	return content
end

function m:move_files( files )
	for to,from in pairs(files) do
		local src = _local(self,from)
		local dst = path.join(self.root,to)
		fs.mkdir_r(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.rename(src,dst))
	end
end

function m:install_script( src_in , dst )
	local src = _local(self,src_in)
	self._project:check_script(dst,self)
	local fdst = self.tosystem and path.join(self.root,'scripts',dst) or 
			path.join(self.root,'build','scripts',dst)
	fs.mkdir_r(path.dirname(fdst))
	log.debug('install',src,'->',fdst)
	fs.unlink(fdst)
	assert(fs.copyfile(src,fdst))
end

function m:install_scripts( dir )
	local ssrc = _local(self,dir)
	local files,err = fs.scanfiles_r(ssrc)
	if not files then
		error(err .. '\n' .. ssrc )
	end
	for _,f in ipairs(files) do
		self._project:check_script(f,self)
		local src = path.join(ssrc,f)
		local dst = self.tosystem and path.join(self.root,'scripts',f) or 
			path.join(self.root,'build','scripts',f)
		fs.mkdir_r(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:install_metas( dir )
	local ssrc = _local(self,dir)
	local files,err = fs.scanfiles_r(ssrc)
	if not files then
		error(err .. '\n' .. ssrc )
	end
	for _,f in ipairs(files) do
		self._project:check_meta(f,self)
		local src = path.join(ssrc,f)
		local dst =  path.join(self.root,'build','lua-meta',f)
		fs.mkdir_r(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:install_scripts_dir( dir )
	local ssrc = _local(self,dir)
	local basename = path.basename(dir)
	local files,err = fs.scanfiles_r(ssrc)
	if not files then
		error(err .. '\n' .. ssrc)
	end
	for _,f in ipairs(files) do
		local fn = path.join(basename,f)
		self._project:check_script(fn,self)
		local src = path.join(ssrc,f)
		local dst = self.tosystem and path.join(self.root,'scripts',fn) or 
			path.join(self.root,'build','scripts',fn)
		fs.mkdir_r(path.dirname(dst))
		log.debug('install',src,'->',dst)
		fs.unlink(dst)
		assert(fs.copyfile(src,dst))
	end
end

function m:foreach_file_r(dir)
	local src = _local(self,dir)
	local files,err = fs.scanfiles_r(src)
	if not files then
		error('failed scan dir ' .. src ..' '.. err)
	end
	local idx = 0
	return function(files)
		while true do		
			idx = idx + 1
			return files[idx]
		end
	end, files
end

function m:foreach_file( dir , recursive )
	local src =  _local(self,dir)
	local files,err = fs.scandir(src)
	if not files then
		error('failed scan dir ' .. src ..' '.. err)
	end
	local idx = 0
	return function(files)
		while true do		
			idx = idx + 1
			local f = files[idx]
			if not f then
				return nil
			end
			if f.isfile then
				return f.name
			end
		end
	end, files
end


function m:preprocess( config )
	local src_file = _local(self,config.src)
	local dst_file = config.insource and _local(self,config.dst) or path.join(self.root,config.dst)

	local data = {}
	local uncomment = config.uncomment or {}
	local comment = config.comment or {}
	local commentline = config.commentline or {}
	local replace = config.replace or {}
	local replace_line = config.replace_line or {}
	local insert_before = config.insert_before or {}

	for line in io.lines(src_file) do 
		--print('process line',line)
		local d,o = string.match(line,'^//#define%s+([^%s]+)(.*)$')
		if d then
			if uncomment[d] then
				line = '#define ' .. d .. o
			end
		else
			-- ft like
			d,o = string.match(line,'^/%*%s*#define%s+([^%s]+)%s+%*/(.*)$')
			if d then
				if uncomment[d] then
					line = '#define ' .. d .. o
				end
			else
				d,o = string.match(line,'^#%s*define%s+([^%s]+)(.*)$')
				if d and comment[d] then
					line = '//#define ' .. d .. o
				elseif d and replace[d] then
					line = '#define ' .. d .. ' ' .. replace[d]
				end
			end
		end
		local pre = insert_before[line]
		if pre then
			table.insert(data,pre)
		end
		if commentline[line] then
			line = '//' .. line
		end
		table.insert(data,replace_line[line] or line)
	end
	fs.mkdir_r(path.dirname(dst_file))
	fs.write_file(dst_file,table.concat(data,'\n'))
	if config.remove_src then
		fs.unlink(src_file)
	end
end

function m:preprocess_am( config )
	local src_file = _local(self,config.src)
	local dst_file = config.insource and _local(self,config.dst) or path.join(self.root,config.dst)

	local data = {}
	local defines = config.defines or {}
	local replace_line = config.replace_line or {}
	local skip = config.skip or {}
	for line in io.lines(src_file) do 
		--print('process line',line)
		local d,o = string.match(line,'^#undef%s+([A-Z_]+)(.*)$')
		if d and not skip[d] then
			local def = defines[d]
			if def then
				if type(def) == 'boolean' then
					line = '#define ' .. d .. o
				else
					line = '#define ' .. d .. ' ' .. def
				end
			else
				line = '//' .. line
			end
		end
		table.insert(data,replace_line[line] or line)
	end
	fs.mkdir_r(path.dirname(dst_file))
	fs.write_file(dst_file,table.concat(data,'\n'))
end

return m