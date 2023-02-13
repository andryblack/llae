name = 'xz'
version = '5.4.1'
archive = name .. '-' .. version .. '.tar.gz'
url = 'https://tukaani.org/xz/' .. archive 
hash = '7e7454778b4cfae238a7660521b29b38'
dir = name .. '-' .. version


dependencies = {
	'llae',
}

function install()
	download(url,archive,hash)
	unpack_tgz(archive)
	
	move_files{
		['build/include/lzma.h'] = 			dir..'/src/liblzma/api/lzma.h',
		['build/include/lzma/version.h'] = 	dir..'/src/liblzma/api/lzma/version.h',
		['build/include/lzma/base.h'] = 	dir..'/src/liblzma/api/lzma/base.h',
		['build/include/lzma/vli.h'] = 		dir..'/src/liblzma/api/lzma/vli.h',
		['build/include/lzma/check.h'] = 	dir..'/src/liblzma/api/lzma/check.h',
		['build/include/lzma/filter.h'] = 	dir..'/src/liblzma/api/lzma/filter.h',
		['build/include/lzma/bcj.h'] = 		dir..'/src/liblzma/api/lzma/bcj.h',
		['build/include/lzma/delta.h'] = 	dir..'/src/liblzma/api/lzma/delta.h',
		['build/include/lzma/lzma12.h'] = 	dir..'/src/liblzma/api/lzma/lzma12.h',
		['build/include/lzma/container.h'] = 	dir..'/src/liblzma/api/lzma/container.h',
		['build/include/lzma/stream_flags.h'] = 	dir..'/src/liblzma/api/lzma/stream_flags.h',
		['build/include/lzma/block.h'] = 	dir..'/src/liblzma/api/lzma/block.h',
		['build/include/lzma/index.h'] = 	dir..'/src/liblzma/api/lzma/index.h',
		['build/include/lzma/index_hash.h'] = 	dir..'/src/liblzma/api/lzma/index_hash.h',
		['build/include/lzma/hardware.h'] = 	dir..'/src/liblzma/api/lzma/hardware.h',
	}

	move_files{
		
	}
end

cmodules = {
	'archive.lzma',
}

build_lib = {
	components = {
		'check','common','delta','lz','lzma','rangecoder', 'simple',
		check = {
			'check.c',
			'check.h',
			'crc_macros.h',
			
			'crc32_table.c',
			'crc32_table_le.h',
			'crc32_table_be.h',
			'crc32_fast.c',

			'crc64_table.c',
			'crc64_table_le.h',
			'crc64_table_be.h',
			'crc64_fast.c',

		},
		common = {
			'common.c',
			'common.h',
			'memcmplen.h', 
			'block_util.c',
			'easy_preset.c',
			'easy_preset.h',
			'filter_common.c',
			'filter_common.h',
			'hardware_physmem.c',
			'index.c',
			'index.h',
			'stream_flags_common.c',
			'stream_flags_common.h',
			'string_conversion.c',
			'vli_size.c',

			'alone_decoder.c',
			'alone_decoder.h',
			'auto_decoder.c',
			'block_buffer_decoder.c',
			'block_decoder.c',
			'block_decoder.h',
			'block_header_decoder.c',
			'easy_decoder_memusage.c',
			'file_info.c',
			'filter_buffer_decoder.c',
			'filter_decoder.c',
			'filter_decoder.h',
			'filter_flags_decoder.c',
			'index_decoder.c',
			'index_decoder.h',
			'index_hash.c',
			'stream_buffer_decoder.c',
			'stream_decoder.c',
			'stream_decoder.h',
			'stream_flags_decoder.c',
			'vli_decoder.c',
		}
	},
	project = [[
		includedirs{
			<%= format_file(module.dir,'src','common') %>,
			<%= format_mod_file(project:get_module('llae'),'src')%>,
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,'src','liblzma',f) %>,<% end %>
			'include',
		}
		defines{
			'HAVE_STDINT_H=1',
			'HAVE_INTTYPES_H=1',
			'HAVE_STDBOOL_H=1',
			'LZMA_API_STATIC=1',
		}
		sysincludedirs {
			'include',
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<% local files = lib.components[f]; if files then for _,ff in ipairs(files) do %>
					<%= format_file(module.dir,'src','liblzma',f , ff) %>,<% end %>
				<% else %>
					<%= format_file(module.dir,'src','liblzma',f , '*.c') %>,
					<%= format_file(module.dir,'src','liblzma',f , '*.h') %>,
				<% end %>
			<% end %>
			<%= format_file(module.dir,'src','liblzma','check' , '*.h') %>,
			<%= format_file(module.dir,'lib','*.h') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','lzma','*.cpp') %>,
			<%= format_mod_file(project:get_module('llae'),'src','modules','lzma','*.h') %>,
		}
		filter {'system:macosx or linux'}
		--	defines{'Z_HAVE_UNISTD_H'}
		filter{}
]]
}
