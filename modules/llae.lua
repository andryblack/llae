name = 'llae'
version = 'develop'

function install()
	download_git('https://github.com/andryblack/llae.git',{branch=version})
	shell[[
cd src
${LLAE_PROJECT_ROOT}/bin/premake5${LLAE_EXE} download || exit 1
${LLAE_PROJECT_ROOT}/bin/premake5${LLAE_EXE} gmake
make -C build config=release
	]]
	install_bin('src/bin/llae')
	install_files{
		['build/premake/llae.lua'] = 	'src/premake/llae.lua',
		['build/premake/lua.lua'] = 	'src/premake/lua.lua',
		['build/premake/libuv.lua'] = 	'src/premake/libuv.lua',
		['build/premake/mbedtls.lua'] = 'src/premake/mbedtls.lua',
		['build/premake/yajl.lua'] = 	'src/premake/yajl.lua',
		['build/premake/utils.lua'] = 	'src/premake/utils.lua',
	}

end


dependencies = {
	'premake',
	'lua'
}

solution = [[
	local llae = require 'premake.llae'
	llae.root = "<%= path.join('modules',name,'src') %>"
	llae.dependencies()
	llae.lib()
]]