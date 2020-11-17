name = 'zlib'
version = '1.2.11'
archive = 'zlib-' .. version .. '.tar.gz'
url = 'https://zlib.net/' .. archive

dir = name .. '-' .. version

function install()
	download(url,archive)

	shell([[
tar -xzf ]] .. archive .. [[ || exit 1
	]])
	move_files{
		['build/include/zlib.h'] = 		dir..'/zlib.h',
		['build/include/zconf.h'] = 	dir..'/zconf.h',
	}
end



build_lib = {
	components = {
		'adler32','crc32','deflate','infback','inffast','inflate','inftrees','trees','zutil',
		'compress','uncompr','gzclose','gzlib','gzread','gzwrite'
	},
	project = [[
		includedirs{
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'src',f .. '.c') %>,<% end %>
		}
		filter {'system:macosx or linux'}
			defines{'Z_HAVE_UNISTD_H'}
		filter{}
]]
}