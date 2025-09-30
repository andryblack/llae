local utils = require 'utils'
local _M = {
	name = 'bzip2',
	version = '1.0.8',
	url = 'https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz',
	archive = 'tar.gz',
}

local components = {
	'bzlib','decompress','compress','randtable','crctable','huffman','blocksort',
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','bzip2-'.._M.version)
	for _,f in ipairs{'bzlib.h',} do
		local src = path.join(_M.root,f)
		utils.install_header(src,f)
	end
	project( 'llae-'.._M.name )
		kind 'StaticLib'
		targetdir 'lib'
		location 'build/project'
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,c..'.c'))
		end
		files(fls)
end


function _M.link(  )
	links{ 'llae-'.._M.name }
end

return _M