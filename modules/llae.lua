name = 'llae'
version = 'develop'

dir = 'llae-src'

function install(tosystem)
	download_git('https://github.com/andryblack/llae.git',{branch=version,dir=dir})
	install_scripts(dir .. '/scripts')
	if tosystem then
		local all_files = {}
		for fn in foreach_file(dir .. '/modules') do
			all_files['modules/' .. fn] = dir .. '/modules/' .. fn
		end
		install_files(all_files)
	end
end

function bootstrap()
	shell [[

premake5$LLAE_EXE --file=<%= dir %>/premake5.lua download
premake5$LLAE_EXE --file=<%= dir %>/premake5.lua gmake
make -C <%= dir %>/build config=release verbose=1
	
	]]

	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	all_files['llae-project.lua'] = dir .. '/data/llae-project.lua' 
	install_files(all_files)

	shell [[
export LUA_PATH="<%= dir %>/tools/?.lua;<%= dir %>/scripts/?.lua"
<%= dir %>/bin/llae-bootstrap install 
<%= dir %>/bin/llae-bootstrap init
premake5$LLAE_EXE --file=build/premake5.lua gmake
make -C build config=release verbose=1
	]]
end

project_config = {
	{'embed_sctipts',type='string',storage='list'},
	{'cmodule',type='string',storage='list'}
}

cmodules = {
	'uv',
	'ssl',
	'json',
	'llae',
	'archive',
	'crypto',
	--'xml'
}

includedir = dir .. '/src' 

dependencies = {
	'premake',
	'lua',
	'libuv',
	'yajl',
	'mbedtls',
	'zlib',
	'pugixml',
}

solution = [[
]]

build_lib = {
	project = [[
		files {
			<%= format_file(module.dir,'src','*/**.h') %>,
			<%= format_file(module.dir,'src','*/**.cpp') %>,
		}
		sysincludedirs {
			'include',
		}
		includedirs{
			<%= format_file(module.dir,'src') %>
		}
	]]
}

project_main = [[
	files {
		<%= format_file(module.dir,'src/main.cpp') %>
	}
	sysincludedirs {
		<%= format_file(module.dir,'src') %>
	}
]]

generate_src = {{
	template = dir .. '/data/embedded-template.cpp',
	filename = 'build/src/embedded-scripts.cpp',
	config = [[
	scripts = {}
	local installed_scripts = {}
	local project_scripts = project:get_config('llae','embed_scripts')
	for _,v in ipairs(project_scripts or {}) do
		local files = fs.scanfiles_r(path.join(project:get_root(),v))
		for __,f in ipairs(files) do
			if path.extension(f) == 'lua' then
				local name = string.gsub(f:sub(1,-5),'/','.')
				installed_scripts[name] = true
				table.insert(scripts,{
					name = name,
					content = fs.load_file(path.join(project:get_root(),v,f))
					})
			end
		end
	end
	if not installed_scripts._main then
		table.insert(scripts,{
			name = '_main',
			content = template.render_file(path.join(location,dir,'data','main-template.lua'),{
			})
		})
	end
	if not installed_scripts['lae.utils'] then
		table.insert(scripts,{
			name = 'lae.utils',
			content = fs.load_file(path.join(location,dir,'scripts','llae/utils.lua'))
		})
	end

	]]
},{
	template = dir .. '/data/embedded-modules.cpp',
	filename = 'build/src/embedded-modules.cpp',
	config = [[
		modules = project:get_cmodules()
	]]
}}
