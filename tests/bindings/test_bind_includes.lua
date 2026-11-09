local lu = require('luaunit')
local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local Project = require('project')
local process_bind = require('cparse.process_bind')
package.path = store_path

TestBindIncludes = {}

function TestBindIncludes:test_project_and_module_dirs()
    local env = {
        location = '/tmp/bind-includes-project',
        project_name = 'test',
        cmdargs = {},
        module_config = {},
        cmodules = {},
        bind_includes = {
            'src',
            './src/',
            '${build_root}/include',
            '${build_root}/include/',
        },
    }
    local prj = Project.new(env)
    prj._modules_list = {
        {
            get_env = function() return { dir = 'lib' } end,
            get_location = function() return '/tmp/bind-includes-module' end,
            get_bind_includes = function() return { 'include', '${dir}/extra' } end,
        },
    }

    local dirs = prj:get_bind_include_dirs()
    lu.assertEquals(dirs, {
        '/tmp/bind-includes-project/src',
        '/tmp/bind-includes-project/build/include',
        '/tmp/bind-includes-module/include',
        '/tmp/bind-includes-module/lib/extra',
    })
end

function TestBindIncludes:test_processor_include_dirs()
    local processor = process_bind.new()
    processor:set_include_dirs({ '/a', '/b' })
    lu.assertEquals(processor:get_include_dirs(), { '/a', '/b' })
end

function TestBindIncludes:test_processor_creates_preprocessor_with_dirs()
    local processor = process_bind.new()
    processor:set_include_dirs({ '/a', '/b' })

    local pp = processor:create_preprocessor()
    lu.assertEquals(pp:get_include_dirs(), { '/a', '/b' })
    lu.assertEquals(type(pp.handle_include), 'function')
end

function TestBindIncludes:test_allowed_include_marker_and_fatal()
    local processor = process_bind.new()
    local pp = processor:create_preprocessor()

    lu.assertNil(pp:handle_include('missing.h', nil, nil))
    lu.assertNil(pp:handle_include('missing.h', nil, 'not allowed'))
    lu.assertNil(pp:handle_include('missing.h', nil, ''))

    local ok, err = pcall(function()
        pp:handle_include('missing.h', nil, '@luabind_allow')
    end)
    lu.assertFalse(ok)
    lu.assertStrContains(tostring(err), 'binding include not found')
    lu.assertStrContains(tostring(err), 'missing.h')
end
