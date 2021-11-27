name = 'bzip2'
version = '1.0.8'
archive = 'bzip2-' .. version .. '.tar.gz'
url = 'https://sourceware.org/pub/bzip2/' .. archive
hash = '67e051268d0c475ea773822f7500d0e5'
dir = name .. '-' .. version

dependencies = {
	'llae',
}

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/bzlib.h'] = 		dir..'/bzlib.h',
	}
end

cmodules = {
	'archive.bzip2',
}

build_lib = {
	components = {
		'bzlib','decompress','compress','randtable','crctable','huffman','blocksort',
	},
	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
			'include',
		}
		defines{
			'BZ_NO_STDIO'
		}
		sysincludedirs {
			'include',
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,f .. '.c') %>,<% end %>
			<%= format_file(module.dir,'*.h') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','bzip2','*.cpp') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','bzip2','*.h') %>,
		}
		filter {'system:macosx or linux'}
			defines{'Z_HAVE_UNISTD_H'}
		filter{}
]]
}
