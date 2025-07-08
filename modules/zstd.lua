name = 'zstd'
version = '1.5.7'
archive = name .. '-' .. version .. '.tar.gz'
url = 'https://github.com/facebook/zstd/releases/download/v' .. version .. '/' .. archive
hash = '780fc1896922b1bc52a4e90980cdda48'
dir = name .. '-' .. version


dependencies = {
	'llae',
}

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	install_files{
		['build/include/zdict.h'] = 		dir..'/lib/zdict.h',
		['build/include/zstd.h'] = 		dir..'/lib/zstd.h',
		['build/include/zstd_errors.h'] = 		dir..'/lib/zstd_errors.h',
	}

end

cmodules = {
	'archive.zstd',
}

build_lib = {
	components = {
		'common/error_private',
		'common/zstd_common',
		'common/entropy_common',
		'common/fse_decompress',
		'common/xxhash',
		'decompress/huf_decompress',
		'decompress/zstd_decompress',
		'decompress/zstd_decompress_block',
		'decompress/zstd_ddict',
		'compress/fse_compress',
		'compress/hist',
		'compress/huf_compress',
		'compress/zstd_compress',
		'compress/zstd_compress_literals',
		'compress/zstd_compress_sequences',
		'compress/zstd_compress_superblock',
		'compress/zstd_double_fast',
		'compress/zstd_fast',
		'compress/zstd_lazy',
		'compress/zstd_ldm',
		'compress/zstd_opt',
		'compress/zstd_preSplit',
	},
	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
			'include',
		}
		sysincludedirs {
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'lib',f .. '.c') %>,<% end %>
			<%= format_mod_file(project:get_module('llae'),'src','modules','zstd','*.cpp') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','zstd','*.h') %>,
		}
		filter{}
	]]
}
