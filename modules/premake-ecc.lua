
name = 'premake-ecc'
revision = 'b3726d5'
version = 'master'
url = 'https://github.com/MattBystrin/premake-ecc/archive/refs/heads/'..version..'.tar.gz'
dir =  name .. '-' .. version 
archive = dir ..  '.tar.gz'
hash = 'c74f63ece0cbcbb37ab0ec498ae2a29f'

function install()
	download(url,archive,hash)
	unpack_tgz(archive,dir,1)
	install_files{ 
        ['build/premake/ecc/ecc.lua'] = dir .. '/ecc.lua',
        ['build/premake/ecc/_manifest.lua'] = dir .. '/_manifest.lua',
        ['build/premake/ecc/_preload.lua'] = dir .. '/_preload.lua',
    }
end

premake_setup = [[
dofile 'premake/ecc/ecc.lua'
]]