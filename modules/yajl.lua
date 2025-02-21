
name = 'yajl'
revision = '66cb08c'
version = '2.1.0'
url = 'https://github.com/lloyd/yajl/archive/refs/tags/'..version..'.tar.gz'
dir =  name .. '-' .. version 
archive = dir ..  '.tar.gz'
hash = '6887e0ed7479d2549761a4d284d3ecb0'

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	install_files{ 
		['build/include/yajl/yajl_common.h'] = 		dir..'/src/api/yajl_common.h',
		['build/include/yajl/yajl_gen.h'] = 		dir..'/src/api/yajl_gen.h',
		['build/include/yajl/yajl_parse.h'] = 		dir..'/src/api/yajl_parse.h',
		['build/include/yajl/yajl_tree.h'] = 		dir..'/src/api/yajl_tree.h',
	}
	-- https://github.com/lloyd/yajl/pull/232/commits/ae1fa8f58491901f071339ced9896ca3ecad0703
	preprocess{
		src = dir .. '/src/yajl_encode.c',
		dst = dir .. '/src/yajl_encode.c',
		insource = true,
		insert_before = {
[ [=[                            unsigned int surrogate = 0;]=] ] = [=[
                        if (str[end + 1] == '\\' && str[end + 2] == 'u') {
]=],
[ [=[                            hexToDigit(&surrogate, str + end + 2);]=] ] = [=[
                            end++;
]=]
		},
		commentline = {
[ [=[                        if (str[end] == '\\' && str[end + 1] == 'u') {]=] ] = true,
[ [=[                        end++;]=] ] = true
		}
	}
end



build_lib = {
	components = {
		'yajl_parser','yajl_buf','yajl_encode','yajl_alloc','yajl_lex'
	},
	main = {
		'yajl','yajl_gen','yajl_tree',
	},
	project = [[
		sysincludedirs{
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