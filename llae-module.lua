name = 'llae'

function install(tosystem)
    install_scripts(dir .. '/scripts')
    install_metas(dir .. '/lua-meta')
end

project_config = {
	{'embed_scripts',type='string',storage='list'},
	{'embed_script',type='string',storage='list'},
	{'extern_main',type='boolean'},
	{'cmodule',type='string',storage='list'},
	{'lua_path',type='string',storage='list'},
	{'lua_main',type='string',storage='list'},
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
	'utf16',
}

includedir = '${dir}/src' 

dependencies = {
	'premake',
	'lua',
	'libuv',
	'yajl',
	'mbedtls',
	'zlib',
	'pugixml',
}

bind_headers = {
	{
		dir = '${dir}/src/crypto',
	},
	{
		dir = '${dir}/src/uv',
	},
	{
		dir = '${dir}/src/net',
	}
}

solution = [[
]]

build_lib = {
	components = {
		'archive','common','crypto','llae','meta','parsers','posix','ssl','uv','lua','net'
	},
	project = [[
		files {
			<% for _,f in ipairs(lib.components) do %>
			<%= format_file(module.dir,'src',f,'**.h') %>,
			<%= format_file(module.dir,'src',f,'**.cpp') %>,<% end %>
		}
		includedirs{
			'include',
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
	includedirs {
		<%= format_file(module.dir,'src') %>
	}
	filter "system:windows"
		links{ 'crypt32' }
	filter {}
]]

generate_src = {{
	template = dir .. '/data/embedded-template.cpp',
	filename = 'build/src/embedded-scripts.cpp',
	config = [[
	scripts = {}
	local installed_scripts = {}
	local project_scripts = project:get_config_value('llae','embed_scripts')
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
	project_scripts = project:get_config_value('llae','embed_script')
	for _,f in ipairs(project_scripts or {}) do
		local name
		local fn = f
		if type(f) == 'table' then
			name = f[1]
			fn = f[2]
		else
			name = string.gsub(f:sub(1,-5),'/','.')
		end
		installed_scripts[name] = true
		local script_path = path.join(project:get_root(),fn)
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
	local force_embedded = {'llae.fs','llae.path','llae.utils','llae.log','llae.uv'}
	for _,v in ipairs(force_embedded) do
		if not installed_scripts[v] then
			log.debug('embed',v)
			local fn = v:gsub('%.','/') .. '.lua'
			table.insert(scripts,{
				name = v,
				content = fs.load_file(path.join(project.get_path(location,dir),'scripts',fn))
			})
		end
	end

	]]
},{
	template = dir .. '/data/embedded-modules.cpp',
	filename = 'build/src/embedded-modules.cpp',
	config = [[
		modules = project:get_cmodules()
	]]
}}


local function get_parallel_opt()
	local parallel = ''
	local arg = project:get_cmdargs()['j']
	if arg then
		parallel = arg
	end
	local parallel_opt = '-j' .. parallel
	return parallel_opt
end	

function bootstrap( config )
	
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
	local cwd = root
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
		args = {'--file=build/premake5.lua','gmake'},
		name = 'bootstrap2_premake',
		env = env,
		cwd = cwd,
	})
	
	assert(exec{
		bin = 'make',
		args = {'-C','build','config=release','verbose=1',get_parallel_opt()},
		name = 'bootstrap2_make',
		env = env,
		cwd = cwd,
	})

end

function upgrade( data )
	
	local install_root = data.install_root
	local all_files = {}
	for fn in foreach_file(dir .. '/data') do
		all_files['data/' .. fn] = dir .. '/data/' .. fn
	end
	all_files['llae-project.lua'] = dir .. '/llae-project.lua' 
	install_files(all_files)

	local env = {
		LLAE_DL_DIR=project:get_dl_dir()
	}
	local cwd = root
	local llae_exe = path.join(root,'bin','llae')
	local llae_root = path.join(cwd,'build','modules','llae',dir)
	assert(exec{
		bin = llae_exe,
		args = {'--root=' .. llae_root,'install'},
		name = 'upgrade_install',
		env = env,
		cwd = cwd,
	})
	assert(exec{
		bin = llae_exe,
		args = {'--root=' .. llae_root,'init'},
		name = 'upgrade_init',
		env = env,
		cwd = cwd,
	})
	local premake_exe = path.join(root,'bin','premake5')
	assert(exec{
		bin = premake_exe,
		args = {'--file=build/premake5.lua','gmake2'},
		name = 'upgrade_premake',
		env = env,
		cwd = cwd,
	})

	assert(exec{
		bin = 'make',
		args = {'-C','build','config=release','verbose=1',get_parallel_opt()},
		name = 'upgrade_make',
		env = env,
		cwd = cwd,
	})
end