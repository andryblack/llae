name = 'llae'
version = 'develop'

dir = 'llae-src'

function install()
	download_git('https://github.com/andryblack/llae.git',{branch=version,dir=dir})
end

function bootstrap()
	shell [[

premake5$LLAE_EXE --file=<%= dir %>/premake5.lua download
premake5$LLAE_EXE --file=<%= dir %>/premake5.lua gmake
make -C <%= dir %>/build config=release
	
	]]
	install_bin(dir .. '/bin/llae')
	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	for fn in foreach_file(dir .. '/modules') do
		all_files['modules/' .. fn] = dir .. '/modules/' .. fn
	end
	install_files(all_files)
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