
local _M = {
	name = 'llae-lua',
	version = '5.3.5',
	url = 'https://www.lua.org/ftp/lua-5.3.5.tar.gz',
	archive = 'tar.gz',
}

local components = {
	'lapi','lauxlib','lcode','lctype','ldebug','ldo','ldump',
	'lfunc','lgc','llex','lmem','lobject','lopcodes','lparser',
	'lstate','lstring','ltable','ltm','lundump','lvm','lzio'
}
local libs = {
	'lbitlib','lcorolib','ldblib','lmathlib','loadlib','loslib',
	'lstrlib','lutf8lib','linit','lbaselib','ltablib','liolib'
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','lua-'.._M.version)
	os.mkdir(path.join(root,'build','include'))
	for _,f in ipairs{'lua.h','luaconf.h','lauxlib.h','lualib.h'} do
		assert(os.copyfile(path.join(_M.root,'src',f),
			path.join(root,'build','include',f)))
	end
	project(_M.name)
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,'src',c..'.c'))
			table.insert(fls,path.join(_M.root,'src',c..'.h'))
		end
		for _,c in ipairs(libs) do
			table.insert(fls,path.join(_M.root,'src',c..'.c'))
		end
		files(fls)
		filter "system:linux or bsd or hurd or aix or haiku"
			defines      { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

		filter "system:macosx"
			defines      { "LUA_USE_MACOSX" }
		filter {}

end

function _M.link(  )
	filter "system:linux"
		links {'dl'}
	filter {}	
	links{ _M.name }
end

return _M
