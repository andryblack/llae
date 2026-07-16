project 'llae'
version = '1.0'

module 'llae'
module 'bzip2'
module 'xz'
module 'zstd'
module 'lz4'

config('llae','embed_scripts','build/scripts')
config('llae','embed_scripts','build/modules/llae/llae-src/tools')

generate_src{
	template = 'data/build_config-template.lua',
	filename = 'build/_build_config.lua',
}
config('llae','embed_script',{'_build_config','build/_build_config.lua'})
