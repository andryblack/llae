project 'llae'
version = '1.0'

self_module 'llae'
module 'bzip2'
module 'xz'
module 'zstd'
module 'lz4'
module 'luaunit'
module 'premake-ecc'
module 'redis'

if cmdargs and (cmdargs.debug or cmdargs.development) then
	config('lua','apicheck',true)
end

config('llae','embed_scripts','scripts')
config('llae','embed_scripts','tools')

if cmdargs and cmdargs.development then
	cmodule 'bind_tests'
	cmodule 'coro_tests'
	cmodule 'result_tests'
	cmodule 'inplace_function_tests'
	premake {
		project = [[
			files{
				<%= format_file('src','tests','*.cpp')%>,
				<%= format_file('src','tests','*.h')%>,
			}
]]
	}
	print("development mode")
	bind_headers('src/tests')
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
