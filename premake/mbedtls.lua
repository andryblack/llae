local _M = {
	name = 'llae-mbedtls',
	version = '2.16.5',
	url = 'https://tls.mbed.org/download/mbedtls-2.16.5-apache.tgz',
	archive = 'tar.gz',
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','mbedtls-'.._M.version)
	os.mkdir(path.join(root,'build','include','mbedtls'))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','mbedtls','*'))) do
		local dst = path.join(root,'build','include','mbedtls',path.getname(f))
		if not os.comparefiles(f,dst) then
			assert(os.copyfile(f,dst))
		end
	end

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