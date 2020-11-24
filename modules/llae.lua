name = 'llae'
version = 'develop'

dir = 'llae-src'

function install(tosystem)
	download_git('https://github.com/andryblack/llae.git',{branch=version,dir=dir})
	install_scripts(dir .. '/scripts')
	if tosystem then
		local all_files = {}
		for fn in foreach_file(dir .. '/modules') do
			all_files['modules/' .. fn] = dir .. '/modules/' .. fn
		end
		install_files(all_files)
	end
end

function bootstrap()
	shell [[

premake5$LLAE_EXE --file=<%= dir %>/premake5.lua download
premake5$LLAE_EXE --file=<%= dir %>/premake5.lua gmake
make -C <%= dir %>/build config=release verbose=1
	
	]]

	install_bin(dir .. '/bin/llae')
	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	install_files(all_files)
end

includedir = dir .. '/src' 

dependencies = {
	'premake',
	'lua',
	'libuv',
	'yajl',
	'mbedtls',
	'zlib'
}

solution = [[
]]

build_lib = {
	project = [[
		files {
			<%= format_file(module.dir,'src','*/**.h') %>,
			<%= format_file(module.dir,'src','*/**.cpp') %>,
		}
		sysincludedirs {
			'include',
		}
		includedirs{
			<%= format_file(module.dir,'src') %>
		}
	]]
}

project_main = [[
	files {
		<%= format_file(module.dir,'src/main.cpp') %>
	}
]]
