name = 'lua'
version = '5.3.5'
archive = 'lua-' .. version .. '.tar.gz'
url = 'https://www.lua.org/ftp/' .. archive


function install()
	download(url,archive)

	shell([[
tar -xzf ]] .. archive .. [[ || exit 1
	]])
	install_files{
		['build/include/lua.h'] = 	'lua-'..version..'/src/lua.h',
		['build/include/lauxlib.h'] = 	'lua-'..version..'/src/lauxlib.h',
		['build/include/lualib.h'] = 	'lua-'..version..'/src/lualib.h',
		['build/include/luaconf.h'] = 	'lua-'..version..'/src/luaconf.h',
	}
	-- preprocess{
	-- 	src = 'lua-'..version .. '/src/luaconf.h',
	-- 	dst = 'build/include/luaconf.h',
	-- 	replace={
	-- 		['LUA_API'] = '',
	-- 		['LUAI_FUNC'] = ''
	-- 	}}
end

local components = {
	'lapi','lauxlib','lcode','lctype','ldebug','ldo','ldump',
	'lfunc','lgc','llex','lmem','lobject','lopcodes','lparser',
	'lstate','lstring','ltable','ltm','lundump','lvm','lzio'
}
local libs = {
	'lbitlib','lcorolib','ldblib','lmathlib','loadlib','loslib',
	'lstrlib','lutf8lib','linit','lbaselib','ltablib','liolib'
}

local fls = {}
for _,c in ipairs(components) do
	table.insert(fls,'lua-'..version..'/src/'..c..'.c')
	table.insert(fls,'lua-'..version..'/src/'..c..'.h')
end
for _,c in ipairs(libs) do
	table.insert(fls,'lua-'..version..'/src/'..c..'.c')
end
build_lib = {
	files = fls,
	project = [[
	includedirs{
		'include'
	}
	filter "system:linux or bsd or hurd or aix or haiku"
		defines      { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

	filter "system:macosx"
		defines      { "LUA_USE_MACOSX" }
	filter {}
]]
}