local _M = {}


function _M.preprocess( src_file, dst_file, config )
	local data = {}
	local uncomment = config.uncomment or {}
	local comment = config.comment or {}
	local replace = config.replace or {}
	for line in io.lines(src_file) do 
		--print('process line',line)
		local d,o = string.match(line,'^//#define%s+([A-Z_]+)(.*)$')
		if d then
			if uncomment[d] then
				print('uncomment',d)
				line = '#define ' .. d .. o
			end
		else
			d,o = string.match(line,'^#%s*define%s+([A-Z_]+)(.*)$')
			if d and comment[d] then
				print('comment',d)
				line = '//#define ' .. d .. o
			elseif d and replace[d] then
				print('replace',d)
				line = '#define ' .. d .. ' ' .. replace[d]
			end
		end
		table.insert(data,line)
	end
	assert(os.writefile_ifnotequal(table.concat(data,'\n'),dst_file))
end

function _M.install_header( src, dst_fn )
	local dst = path.join('build','include',dst_fn)
	if not os.comparefiles(src,dst) then
		os.mkdir(path.getdirectory(dst))
	
		print('install',src,'->',dst)
		assert(os.copyfile(src,dst))
	end
end

return _M