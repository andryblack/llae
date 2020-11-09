
name = 'yajl'
revision = '66cb08c'
version = '2.1.0'
url = 'https://github.com/lloyd/yajl/tarball/'..version
dir =  name .. '-' .. version 
archive = dir ..  '.tar.gz'


function install()
	download(url,archive)

	shell([[
tar -xzf ]] .. archive .. [[ || exit 1
mv lloyd-yajl-]] .. revision .. ' ' .. dir .. [[ || exit 1
	]])
	install_files{ 
		['build/include/yajl/yajl_common.h'] = 		dir..'/src/api/yajl_common.h',
		['build/include/yajl/yajl_gen.h'] = 		dir..'/src/api/yajl_gen.h',
		['build/include/yajl/yajl_parse.h'] = 		dir..'/src/api/yajl_parse.h',
		['build/include/yajl/yajl_tree.h'] = 		dir..'/src/api/yajl_tree.h',
	}
	-- preprocess{
	-- 	src = 'lua-'..version .. '/src/luaconf.h',
	-- 	dst = 'build/include/luaconf.h',
	-- 	replace={
	-- 		['LUA_API'] = '',
	-- 		['LUAI_FUNC'] = ''
	-- 	}}
end



build_lib = {
	components = {
		'yajl_parser','yajl_buf','yajl_encode','yajl_alloc','yajl_lex'
	},
	main = {
		'yajl','yajl_gen','yajl_tree',
	},
	project = [[
		includedirs{
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'src',f .. '.c') %>,
				<%= format_file(module.dir,'src',f .. '.h') %>,<% end %>
			<% for _,f in ipairs(lib.main) do %>
				<%= format_file(module.dir,'src',f .. '.c') %>,<% end %>
		}
]]
}