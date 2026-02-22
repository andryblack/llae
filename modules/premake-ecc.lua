
name = 'premake-ecc'
revision = '47e3ee063728a834cf98cd73e7281057c099d1cb'
version = 'tmp/externalincludedirs'
url = 'https://github.com/andryblack/premake-ecc/archive/refs/heads/'..version..'.tar.gz'
dir =  name .. '-' .. string.gsub(version,'/','_') 
archive = dir ..  '.tar.gz'
hash = '49c51bca5d743c8eefb2aa3090097a2c'

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