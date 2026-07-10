local log = require 'llae.log'

local embedded = {
    llae = {
        url = 'github.com/andryblack/llae.git',
        branch = 'develop',
        proto = 'https',
        dir = 'llae-src',
    }
}

function embedded.get_module_llae(project,install)
    local modules_git = require 'modules.git'
    local mod,err = modules_git.load(project,embedded.llae.url .. ';branch=' .. embedded.llae.branch .. ';proto=' .. embedded.llae.proto .. ';dir=' .. embedded.llae.dir,install)
    if mod then
        return mod
    end
    return nil, err
end

function embedded.get(project,name,install)
	local func = embedded['get_module_' .. name]
	if func then
        log.debug('load embedded module',name)
		return func(project,install)
	end
	return nil, 'not found embedded module: ' .. name
end

function embedded.bootstrap()
    local mf = require 'modules.functions'
    local env = {}

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

return embedded