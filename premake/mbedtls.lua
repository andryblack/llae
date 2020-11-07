local utils = require 'utils'

local _M = {
	name = 'llae-mbedtls',
	version = '2.16.5',
	url = 'https://tls.mbed.org/download/mbedtls-2.16.5-apache.tgz',
	archive = 'tar.gz',
}

local uncomment = {
	['MBEDTLS_DEPRECATED_REMOVED'] = true,
	['MBEDTLS_HAVE_SSE2'] = true,
	['MBEDTLS_PLATFORM_PRINTF_ALT'] = true,
	['MBEDTLS_KEY_EXCHANGE_PSK_ENABLED'] = true,
	['MBEDTLS_SSL_PROTO_TLS1_2'] = true,
}
local comment = {
	['MBEDTLS_NO_UDBL_DIVISION'] = true,
	['MBEDTLS_NET_C'] = true
}


function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','mbedtls-'.._M.version)
	os.mkdir(path.join(root,'build','include','mbedtls'))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','mbedtls','*'))) do
		local n = path.getname(f)
		if n ~= 'config.h' then
			utils.install_header(f,path.join('mbedtls',n))
		end
	end

	utils.preprocess(
		path.join(_M.root,'include','mbedtls','config.h'),
		path.join('include','mbedtls','config.h'),
		{uncomment = uncomment, comment = comment })

	-- for _,f in ipairs{'lua.h','luaconf.h','lauxlib.h','lualib.h'} do
	-- 	assert(os.copyfile(path.join(_M.root,'src',f),
	-- 		path.join(root,'build','include',f)))
	-- end
	project(_M.name)
		kind 'StaticLib'
		targetdir 'lib'
		location 'project'

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