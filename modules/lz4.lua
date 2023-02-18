name = 'lz4'
version = '1.9.4'
archive = name .. '-' .. version .. '.tar.gz'
url = 'https://github.com/lz4/lz4/archive/refs/tags/v' .. version .. '.tar.gz'
hash = 'e9286adb64040071c5e23498bf753261'
dir = name .. '-' .. version

dependencies = {
	'llae',
}

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/lz4.h'] = 		dir..'/lib/lz4.h',
		['build/include/lz4frame.h'] = 	dir..'/lib/lz4frame.h',
		['build/include/lz4hc.h'] = 	dir..'/lib/lz4hc.h',
	}

	move_files{
		
	}
end

cmodules = {
	'archive.lz4',
}

build_lib = {
	components = {
		'lz4','lz4frame','lz4hc','xxhash'
	},
	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
			'include',
		}
		defines{
			'LZ4LIB_VISIBILITY=',
			'HAVE_DECODER_LZMA1=1',
			'HAVE_DECODER_LZMA2=1',
		}
		sysincludedirs {
			'include',
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'lib',f .. '.c') %>,<% end %>
			<%= format_file(module.dir,'lib','*.h') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','lz4','*.cpp') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','lz4','*.h') %>,
		}
		filter {'system:macosx or linux'}
		--	defines{'Z_HAVE_UNISTD_H'}
		filter{}
]]
}
