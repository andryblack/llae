name = 'llae'
version = 'develop'

dir = 'llae-src'

local inplace = project:get_cmdargs().inplace
if inplace and tostring(inplace) == 'true' then
 	dir = '../../..'
 	inplace_dir = '..'
elseif inplace then
	dir = inplace
	inplace_dir = inplace
end 

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
	
	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	all_files['llae-project.lua'] = dir .. '/llae-project.lua' 
	install_files(all_files)
	local env = {
		LLAE_DL_DIR=project:get_dl_dir()
	}
	env.LUA_PATH = get_absolute_location(dir,'tools','?.lua') .. ';' .. get_absolute_location(dir,'scripts','?.lua')
	cwd = root
	local bootstrap = get_self_exe()
	assert(exec{
		bin = bootstrap,
		args = {'install'},
		name = 'bootstrap2_install',
		env = env,
		cwd = cwd,
	})
	assert(exec{
		bin = bootstrap,
		args = {'init'},
		name = 'bootstrap2_init',
		env = env,
		cwd = cwd,
	})
	env.LUA_PATH='?.lua'
	assert(exec{
		bin = 'premake5',
		args = {'--file=build/premake5.lua','gmake2'},
		name = 'bootstrap2_premake',
		env = env,
		cwd = cwd,
	})
	assert(exec{
		bin = 'make',
		args = {'-C','build','config=release','verbose=1','-j4'},
		name = 'bootstrap2_make',
		env = env,
		cwd = cwd,
	})

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
	{'embed_sctipt',type='string',storage='list'},
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
	'posix',
	'posix.termios',
	'net',
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
		'archive','common','crypto','llae','meta','parsers','posix','ssl','uv','lua','net'
	},
	project = inplace and [[
		files {
			<% for _,f in ipairs(lib.components) do %>
			'<%= path.join(module.inplace_dir,'src',f,'**.h') %>',
			'<%= path.join(module.inplace_dir,'src',f,'**.cpp') %>',<% end %>
		}
		sysincludedirs {
			'include',
		}
		includedirs{
			'<%= path.join(module.inplace_dir,'src') %>',
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
	project_scripts = project:get_config('llae','embed_script')
	for _,f in ipairs(project_scripts or {}) do
		local name = string.gsub(f:sub(1,-5),'/','.')
		installed_scripts[name] = true
		local script_path = path.join(project:get_root(),f)
		log.debug('embed',script_path,name)
		table.insert(scripts,{
			name = name,
			content = fs.load_file(script_path)
			})
	end
	if not installed_scripts._main then
		local content = template.render_file(path.join(project.get_path(location,dir),'data','main-template.lua'),{
			project = project
			})
		log.debug('embed','_main',content)
		table.insert(scripts,{
			name = '_main',
			content = content,	
		})
	end
	if not installed_scripts['llae.fs'] then
		log.debug('embed','llae.fs')
		table.insert(scripts,{
			name = 'llae.fs',
			content = fs.load_file(path.join(project.get_path(location,dir),'scripts','llae/fs.lua'))
		})
	end
	if not installed_scripts['llae.path'] then
		log.debug('embed','llae.path')
		table.insert(scripts,{
			name = 'llae.path',
			content = fs.load_file(path.join(project.get_path(location,dir),'scripts','llae/path.lua'))
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
