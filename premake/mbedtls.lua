local utils = require 'utils'

local _M = {
	name = 'mbedtls',
	version = '3.0.0',
	url = 'https://github.com/ARMmbed/mbedtls/archive/refs/tags/v3.0.0.tar.gz',
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
	['MBEDTLS_NET_C'] = true,
	['MBEDTLS_FS_IO'] = true,
	['MBEDTLS_PSA_ITS_FILE_C'] = true,
	['MBEDTLS_PSA_CRYPTO_STORAGE_C'] = true,
}


function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','mbedtls-'.._M.version)
	os.mkdir(path.join(root,'build','include','mbedtls'))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','mbedtls','*'))) do
		local n = path.getname(f)
		if n ~= 'mbedtls_config.h' then
			utils.install_header(f,path.join('mbedtls',n))
		end
	end
	os.mkdir(path.join(root,'build','include','psa'))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','psa','*'))) do
		local n = path.getname(f)
		--if n ~= 'mbedtls_config.h' then
			utils.install_header(f,path.join('psa',n))
		--end
	end

	utils.preprocess(
		path.join(_M.root,'include','mbedtls','mbedtls_config.h'),
		path.join(root,'build','include','mbedtls','mbedtls_config.h'),
		{uncomment = uncomment, comment = comment })

	-- for _,f in ipairs{'lua.h','luaconf.h','lauxlib.h','lualib.h'} do
	-- 	assert(os.copyfile(path.join(_M.root,'src',f),
	-- 		path.join(root,'build','include',f)))
	-- end
	project('llae-'.._M.name)
		kind 'StaticLib'
		targetdir 'lib'
		location 'build/project'

		includedirs{
			path.join(root,'build','include'),
			path.join(_M.root,'library')
		}
		files{
			path.join(_M.root,'library','*.c')
		}
		

end

function _M.link(  )
	links{ 'llae-'.._M.name }
end

return _M