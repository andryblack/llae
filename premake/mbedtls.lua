local _M = {
	name = 'llae-mbedtls',
	version = '2.16.5',
	url = 'https://tls.mbed.org/download/mbedtls-2.16.5-apache.tgz',
	archive = 'tar.gz',
}

local uncomment = {
	['MBEDTLS_DEPRECATED_REMOVED'] = true,
	['MBEDTLS_HAVE_SSE2'] = true,
	['MBEDTLS_PLATFORM_PRINTF_ALT'] = true
}
local comment = {
	['MBEDTLS_NO_UDBL_DIVISION'] = true,
	['MBEDTLS_NET_C'] = true
}
local function process_config( src_path, dst_path )
	local data = {}
	for line in io.lines(src_path) do 
		--print('process line',line)
		local d,o = string.match(line,'^//#define ([A-Z_]+)(.*)$')
		if d then
			if uncomment[d] then
				print('uncomment',d)
				line = '#define ' .. d .. o
			end
		else
			d,o = string.match(line,'^#define ([A-Z_]+)(.*)$')
			if d and comment[d] then
				print('comment',d)
				line = '//#define ' .. d .. o
			end
		end
		table.insert(data,line)
	end
	assert(os.writefile_ifnotequal(table.concat(data,'\n'),dst_path))
end

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','mbedtls-'.._M.version)
	os.mkdir(path.join(root,'build','include','mbedtls'))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','mbedtls','*'))) do
		local n = path.getname(f)
		if n ~= 'config.h' then
			local dst = path.join(root,'build','include','mbedtls',n)
			if not os.comparefiles(f,dst) then
				assert(os.copyfile(f,dst))
			end
		end
	end

	process_config(
		path.join(_M.root,'include','mbedtls','config.h'),
		path.join(root,'build','include','mbedtls','config.h'))

	-- for _,f in ipairs{'lua.h','luaconf.h','lauxlib.h','lualib.h'} do
	-- 	assert(os.copyfile(path.join(_M.root,'src',f),
	-- 		path.join(root,'build','include',f)))
	-- end
	project(_M.name)
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		includedirs{
			path.join(root,'build','include')
		}
		files{
			path.join(_M.root,'library','*.c')
		}
		-- local fls = {}
		-- for _,c in ipairs(components) do
		-- 	table.insert(fls,path.join(_M.root,'src',c..'.c'))
		-- 	table.insert(fls,path.join(_M.root,'src',c..'.h'))
		-- end
		-- for _,c in ipairs(libs) do
		-- 	table.insert(fls,path.join(_M.root,'src',c..'.c'))
		-- end
		-- files(fls)
		-- filter "system:linux or bsd or hurd or aix or haiku"
		-- 	defines      { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

		-- filter "system:macosx"
		-- 	defines      { "LUA_USE_MACOSX" }
		-- filter {}

end

function _M.link(  )
	links{ _M.name }
end

return _M