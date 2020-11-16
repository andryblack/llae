name = 'lua'
version = '5.3.5'
archive = 'lua-' .. version .. '.tar.gz'
url = 'https://www.lua.org/ftp/' .. archive

dir = name .. '-' .. version

function install()
	download(url,archive)

	shell([[
tar -xzf ]] .. archive .. [[ || exit 1
	]])
	move_files{
		['build/include/lua.h'] = 		dir..'/src/lua.h',
		['build/include/lauxlib.h'] = 	dir..'/src/lauxlib.h',
		['build/include/lualib.h'] = 	dir..'/src/lualib.h',
	}
	preprocess{
		src = dir .. '/src/luaconf.h',
		dst = 'build/include/luaconf.h',
		remove_src = true,
		replace_line = {
			['/* #define LUA_USE_C89 */'] = [[

#if defined(LUA_EXTRA_CONF)
#include LUA_EXTRA_CONF
#endif
			]]
		}
	}
end



build_lib = {
	components = {
		'lapi','lauxlib','lcode','lctype','ldebug','ldo','ldump',
		'lfunc','lgc','llex','lmem','lobject','lopcodes','lparser',
		'lstate','lstring','ltable','ltm','lundump','lvm','lzio'
	},
	libs = {
		'lbitlib','lcorolib','ldblib','lmathlib','loadlib','loslib',
		'lstrlib','lutf8lib','linit','lbaselib','ltablib','liolib'
	},
	project = [[
		includedirs{
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'src',f .. '.c') %>,
				<%= format_file(module.dir,'src',f .. '.h') %>,<% end %>
			<% for _,f in ipairs(lib.libs) do %>
				<%= format_file(module.dir,'src',f .. '.c') %>,<% end %>
		}
		filter "system:linux or bsd or hurd or aix or haiku"
			defines      { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

		filter "system:macosx"
			defines      { "LUA_USE_MACOSX" }
		filter {}
]]
}