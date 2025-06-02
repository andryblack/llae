project 'llae'

module 'llae'
module 'bzip2'
module 'xz'

if cmdargs and (cmdargs.development or cmdargs.inplace) then
	module 'luaunit'
end

if cmdargs and cmdargs.debug then
	config('lua','apicheck',true)
end

if cmdargs and cmdargs.inplace then
	config('llae','embed_scripts','scripts')
	config('llae','embed_scripts','tools')
	module 'redis'
	module 'luaunit'
else
	config('llae','embed_scripts','build/scripts')
	config('llae','embed_scripts','build/modules/llae/llae-src/tools')
end

generate_src{
	template = 'data/build_config-template.lua',
	filename = 'build/_build_config.lua',
}
config('llae','embed_script',{'_build_config','build/_build_config.lua'})

command {
	name = 'tests',
	script = 'tests/run_all.lua'
}
