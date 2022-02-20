name = 'llae'
version = 'develop'

dir = project:get_cmdargs().inplace and '../../..' or 'llae-src'

function install(tosystem)
	if not project:get_cmdargs().inplace then

		download_git('https://github.com/andryblack/llae.git',{branch=version,dir=dir})
		install_scripts(dir .. '/scripts')
		if tosystem then
			local all_files = {}
			for fn in foreach_file(dir .. '/modules') do
				all_files['modules/' .. fn] = dir .. '/modules/' .. fn
			end
			install_files(all_files)
		end
	else
		log.info('skip install')
	end
end

function bootstrap()
	shell [[
echo "build llae-bootstrap at $PWD/<%= dir %>"
cd <%= dir %>
export LUA_PATH='?.lua'
premake5$LLAE_EXE download || exit 1
premake5$LLAE_EXE unpack || exit 1
premake5$LLAE_EXE gmake2 || exit 1
make -C build config=release verbose=1 || exit 1
	
	]]

	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	all_files['llae-project.lua'] = dir .. '/llae-project.lua' 
	install_files(all_files)

	shell [[
CURDIR=$PWD
export LUA_PATH="$CURDIR/<%= dir %>/tools/?.lua;$CURDIR/<%= dir %>/scripts/?.lua"
cd $LLAE_PROJECT_ROOT
$CURDIR/<%= dir %>/bin/llae-bootstrap install || exit 1
$CURDIR/<%= dir %>/bin/llae-bootstrap init || exit 1
export LUA_PATH='?.lua'
premake5$LLAE_EXE --file=build/premake5.lua gmake2 || exit 1
make -C build config=release verbose=1 || exit 1
	]]
end

function upgrade()
	
	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	all_files['llae-project.lua'] = dir .. '/llae-project.lua' 
	install_files(all_files)

	shell [[
CURDIR=$PWD
cd $LLAE_PROJECT_ROOT
./bin/llae --root=$CURDIR/<%= dir %> install || exit 1
./bin/llae --root=$CURDIR/<%= dir %> init || exit 1
premake5$LLAE_EXE --file=build/premake5.lua gmake2 || exit 1
make -C build config=release verbose=1 || exit 1
	]]
end

project_config = {
	{'embed_sctipts',type='string',storage='list'},
	{'extern_main',type='boolean'},
	{'cmodule',type='string',storage='list'},
	{'lua_path',type='string',storage='list'},
}

cmodules = {
	'uv',
	'ssl',
	'json',
	'llae',
	'archive',
	'crypto',
	'xml',
	'posix'
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
	components = {
		'archive','common','crypto','llae','meta','parsers','posix','ssl','uv','lua'
	},
	project = project:get_cmdargs().inplace  and [[
		files {
			<% for _,f in ipairs(lib.components) do %>
			'<%= path.join('..','src',f,'**.h') %>',
			'<%= path.join('..','src',f,'**.cpp') %>',<% end %>
		}
		sysincludedirs {
			'include',
		}
		includedirs{
			'<%= path.join('..','src') %>',
		}
	]] or [[
		files {
			<% for _,f in ipairs(lib.components) do %>
			<%= format_file(module.dir,'src',f,'**.h') %>,
			<%= format_file(module.dir,'src',f,'**.cpp') %>,<% end %>
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
	<% if not project:get_config_value('llae','extern_main') then %>
	files {
		<%= format_file(module.dir,'src/main.cpp') %>
	}
	<% end %>
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
				local script_path = path.join(project:get_root(),v,f)
				log.debug('embed',script_path,name)
				table.insert(scripts,{
					name = name,
					content = fs.load_file(script_path)
					})
			end
		end
	end
	if not installed_scripts._main then
		local content = template.render_file(path.join(location,dir,'data','main-template.lua'),{
			project = project
			})
		log.debug('embed','_main',content)
		table.insert(scripts,{
			name = '_main',
			content = content,	
		})
	end
	if not installed_scripts['llae.utils'] then
		log.debug('embed','llae.utils')
		table.insert(scripts,{
			name = 'llae.utils',
			content = fs.load_file(path.join(location,dir,'scripts','llae/utils.lua'))
		})
	end
	if not installed_scripts['llae.path'] then
		log.debug('embed','llae.path')
		table.insert(scripts,{
			name = 'llae.path',
			content = fs.load_file(path.join(location,dir,'scripts','llae/path.lua'))
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
