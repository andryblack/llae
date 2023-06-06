name = 'mbedtls'
version = '3.4.0'
dir = name .. '-' .. version 
archive = dir .. '.tar.gz'
url = 'https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v'..version..'.tar.gz'
hash = '3f6c2eadc1243e9895d65c67b46eb890'


local uncomment = {
	['MBEDTLS_DEPRECATED_REMOVED'] = true,
	['MBEDTLS_HAVE_SSE2'] = true,
	['MBEDTLS_PLATFORM_PRINTF_ALT'] = true,
	['MBEDTLS_KEY_EXCHANGE_PSK_ENABLED'] = true,
	['MBEDTLS_SSL_PROTO_TLS1_2'] = true,
	['MBEDTLS_SSL_PROTO_TLS1_3'] = true,
	['MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE'] = true,
}
local comment = {
	['MBEDTLS_NO_UDBL_DIVISION'] = true,
	['MBEDTLS_NET_C'] = true,
	['MBEDTLS_FS_IO'] = true,
	['MBEDTLS_PSA_ITS_FILE_C'] = true,
	['MBEDTLS_PSA_CRYPTO_STORAGE_C'] = true,
	['MBEDTLS_AESCE_C'] = true,
	['MBEDTLS_SSL_SRV_C'] = true,
}
local replace_line = {
	['#define MBEDTLS_HAVE_ASM'] = [[

#ifndef __clang__
#define MBEDTLS_HAVE_ASM
#endif

]],
['#define MBEDTLS_AESNI_C'] = [[

#ifndef __clang__
#define MBEDTLS_AESNI_C
#endif

]],
['#define MBEDTLS_PADLOCK_C'] = [[

#ifndef __clang__
#define MBEDTLS_AESNI_C
#endif

]]
}

function install()
	download(url,archive,hash)

	unpack_tgz(archive,dir,1)

	local includes = {}
	for fn in foreach_file(dir .. '/include/mbedtls') do
		includes['build/include/mbedtls/' .. fn] = dir .. '/include/mbedtls/' .. fn
	end
	for fn in foreach_file(dir .. '/include/psa') do
		includes['build/include/psa/' .. fn] = dir .. '/include/psa/' .. fn
	end
	install_files(includes)
-- 

	for _,v in ipairs(project:get_config_value(name,'uncomment') or {}) do
		uncomment[v]=true
	end 

	preprocess{
		src =  dir .. '/include/mbedtls/mbedtls_config.h',
		dst = 'build/include/mbedtls/mbedtls_config.h',
		uncomment = uncomment,
		comment = comment,
		replace_line = replace_line
	}
end

project_config = {
	{'uncomment',type='string',storage='list'},
}


build_lib = {
	project = [[
		includedirs{
			'include',
			<%= format_file(module.dir,'library') %>
		}
		sysincludedirs {
			'include',
			<%= format_file(module.dir,'library') %>
		}
		files {
			<%= format_file(module.dir,'library','*.c') %>
		}
]]
}