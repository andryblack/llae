name = 'llae'
version = 'develop'

dir = 'llae-src'

function install()
	download_git('https://github.com/andryblack/llae.git',{branch=version,dir=dir})
end


dependencies = {
	'premake',
	'lua',
	'libuv',
	'yajl',
	'mbedtls',
}

solution = [[
]]

build_lib = {
	project = [[
		files {
			<%= format_file(module.dir,'src','**.h') %>,
			<%= format_file(module.dir,'src','**.cpp') %>,
		}
		sysincludedirs {
			'include',
		}
		includedirs{
			<%= format_file(module.dir,'src') %>
		}
	]]
}