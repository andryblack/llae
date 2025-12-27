name = 'zlib'
version = '1.3.1'
archive = 'zlib-' .. version .. '.tar.gz'
url = 'https://zlib.net/fossils/' .. archive
hash = '9855b6d802d7fe5b7bd5b196a2271655'
dir = name .. '-' .. version

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/llae-private/zlib.h'] = 		dir..'/zlib.h',
		['build/include/llae-private/zconf.h'] = 	dir..'/zconf.h',
	}
end



build_lib = {
	components = {
		'adler32','crc32','deflate','infback','inffast','inflate','inftrees','trees','zutil',
		'compress','uncompr',
	},
	project = [[
		includedirs{
			'include/llae-private'
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