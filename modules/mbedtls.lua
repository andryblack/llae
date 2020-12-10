name = 'mbedtls'
version = '2.16.5'
dir = name .. '-' .. version 
archive = dir .. '.tar.gz'
url = 'https://tls.mbed.org/download/' .. dir .. '-apache.tgz'

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


function install()
	download(url,archive)

	unpack_tgz(archive)

	local includes = {}
	for fn in foreach_file(dir .. '/include/mbedtls') do
		includes['build/include/mbedtls/' .. fn] = dir .. '/include/mbedtls/' .. fn
	end
	install_files(includes)
-- 
	preprocess{
		src =  dir .. '/include/mbedtls/config.h',
		dst = 'build/include/mbedtls/config.h',
		uncomment = uncomment,
		comment = comment
	}
end

build_lib = {
	project = [[
		includedirs{
			'include'
		}
		files {
			<%= format_file(module.dir,'library','*.c') %>
		}
]]
}