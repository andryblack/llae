name = 'lua'
version = '5.3.5'
archive = 'lua-' .. version .. '.tar.gz'
url = 'https://www.lua.org/ftp/' .. archive
hash = '4f4b4f323fd3514a68e0ab3da8ce3455'

dir = name .. '-' .. version

project_config = {
	{'apicheck',type='boolean'},
}

function install()
	download(url,archive,hash)

	unpack_tgz(archive)
	
	move_files{
		['build/include/lua.h'] = 		dir..'/src/lua.h',
		['build/include/lauxlib.h'] = 	dir..'/src/lauxlib.h',
		['build/include/lualib.h'] = 	dir..'/src/lualib.h',
	}
	
	log.info('apicheck:',project:get_config_value('lua','apicheck'))
	preprocess{
		src = dir .. '/src/luaconf.h',
		dst = 'build/include/luaconf.h',
		remove_src = true,
		replace_line = {
			['/* #define LUA_USE_C89 */'] = project:get_config_value('lua','apicheck') and [[

/* #define LUA_USE_C89 */

#define LUA_USE_APICHECK

]]
		}
	}
end



build_lib = {
	components = {
		'lapi','lcode','lctype','ldebug','ldo','ldump',
		'lfunc','lgc','llex','lmem','lobject','lopcodes','lparser',
		'lstate','lstring','ltable','ltm','lundump','lvm','lzio'
	},
	libs = {
		'lbitlib','lcorolib','ldblib','lmathlib','loadlib','loslib',
		'lstrlib','lutf8lib','linit','lbaselib','ltablib','liolib',
		'lauxlib',
	},
	project = [[
		includedirs{
			'include'
		}
		files {
			'include/lua.h',
			'include/lauxlib.h',
			'include/lualib.h',
			'include/luaconf.h',
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

project_main = [[
	filter "system:linux"
		links{ 'dl' }
	filter {}
]]
