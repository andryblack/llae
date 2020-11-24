local utils = require 'utils'
local _M = {
	name = 'llae-zlib',
	version = '1.2.11',
	url = 'https://zlib.net/zlib-1.2.11.tar.gz',
	archive = 'tar.gz',
}

local components = {
	'adler32','crc32','deflate','infback','inffast','inflate','inftrees','trees','zutil',
		'compress','uncompr','gzclose','gzlib','gzread','gzwrite'
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','zlib-'.._M.version)
	for _,f in ipairs{'zlib.h','zconf.h',} do
		local src = path.join(_M.root,f)
		utils.install_header(src,f)
	end
	project(_M.name)
		kind 'StaticLib'
		targetdir 'lib'
		location 'build/project'
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,c..'.c'))
		end
		files(fls)
		filter {'system:macosx or linux'}
			defines{'Z_HAVE_UNISTD_H'}
		filter{}
end


function _M.link(  )
	links{ _M.name }
end

return _M