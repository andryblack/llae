
local _M = {
	name = 'llae-yajl',
	revision = '66cb08c',
	version = '2.1.0',
	url = 'https://codeload.github.com/lloyd/yajl/legacy.zip/2.1.0',
	archive = 'zip',
}

local components = {
	'yajl_parser','yajl_buf','yajl_encode','yajl_alloc','yajl_lex'
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','lloyd-yajl-'.._M.revision)
	os.mkdir(path.join(root,'build','include','yajl'))
	for _,f in ipairs{'yajl_common.h','yajl_gen.h','yajl_parse.h','yajl_tree.h'} do
		local src = path.join(_M.root,'src','api',f)
		local dst = path.join(root,'build','include','yajl',f)
		if not os.comparefiles(src,dst) then
			assert(os.copyfile(src,dst))
		end
	end
	project(_M.name)
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		sysincludedirs {'build/include'}
		
		local fls = {
			path.join(_M.root,'src','yajl.c'),
			path.join(_M.root,'src','yajl_tree.c'),
			path.join(_M.root,'src','yajl_gen.c'),
			--path.join(_M.root,'src','version.c')
		}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,'src',c..'.c'))
			table.insert(fls,path.join(_M.root,'src',c..'.h'))
		end
		files(fls)
end


function _M.link(  )
	links{ _M.name }
end

return _M