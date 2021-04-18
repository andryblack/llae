
name = 'pugixml'
version = '1.11'
dir =  name .. '-' .. version 
archive = dir ..  '.tar.gz'
url = 'http://github.com/zeux/pugixml/releases/download/v'..version..'/'..archive
hash = '93540f4644fd4e4b02049554ef37fb90'

local uncomment = {
	['PUGIXML_NO_EXCEPTIONS'] = true,
}
function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/pugixml/pugixml.hpp'] = 		dir..'/src/pugixml.hpp',
	}

	preprocess{
		src =  dir .. '/src/pugiconfig.hpp',
		dst = 'build/include/pugixml/pugiconfig.hpp',
		uncomment = uncomment,
		comment = {},
		remove_src = true
	}
end



build_lib = {
	project = [[
		includedirs{
			'include/pugixml'
		}
		files {
			<%= format_file(module.dir,'src','pugixml.cpp') %>
		}
]]
}