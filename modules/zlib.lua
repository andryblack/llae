name = 'zlib'
version = '1.2.13'
archive = 'zlib-' .. version .. '.tar.gz'
url = 'https://zlib.net/fossils/' .. archive
hash = '9b8aa094c4e5765dabf4da391f00d15c'
dir = name .. '-' .. version

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/zlib.h'] = 		dir..'/zlib.h',
		['build/include/zconf.h'] = 	dir..'/zconf.h',
	}
end



build_lib = {
	components = {
		'adler32','crc32','deflate','infback','inffast','inflate','inftrees','trees','zutil',
		'compress','uncompr',
	},
	project = [[
		includedirs{
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,f .. '.c') %>,<% end %>
		}
		filter {'system:macosx or linux'}
			defines{'Z_HAVE_UNISTD_H'}
		filter{}
]]
}