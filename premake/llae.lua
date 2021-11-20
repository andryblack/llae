
local _M = {}

_M.root = _MAIN_SCRIPT_DIR


local components = { 'common','lua','meta','uv','llae','ssl','archive','crypto','posix' }

local extlibs = {
	require 'lua',
	require 'libuv',
	require 'mbedtls',
	require 'zlib'
}

function _M.extract_zip( zip_file, extlibs_folder )
	assert(zip.extract(zip_file, extlibs_folder))
end

function _M.extract_tar_gz( zip_file, extlibs_folder )
	local cmd = 'tar -xzf ' .. zip_file .. ' -C ' .. extlibs_folder
	assert(os.execute(cmd))
end

function _M.download(  )
	local extlibs_folder = path.join(_M.root,'build','extlibs')
	local dl_folder = os.getenv('LLAE_DL_DIR') or path.join(_M.root,'dl')
	os.mkdir(dl_folder)
	for _,ext in ipairs(extlibs) do
		if ext.url then
			local zip_file = path.join(dl_folder,ext.name..'-'..ext.version) .. '.' .. ext.archive 
			if not os.isfile(zip_file) then
				local res,code = http.download(ext.url,
					zip_file,
					{})
				if res ~= 'OK' then
					error('failed download ' .. ext.name .. ' ' .. res)
				end
			end
		end
	end
end

function _M.unpack()
	local extlibs_folder = path.join(_M.root,'build','extlibs')
	local dl_folder = os.getenv('LLAE_DL_DIR') or path.join(_M.root,'dl')
	os.mkdir(extlibs_folder)
	for _,ext in ipairs(extlibs) do
		if ext.url then
			local zip_file = path.join(dl_folder,ext.name..'-'..ext.version) .. '.' .. ext.archive 
			_M['extract_'..string.gsub(ext.archive,'%.','_')](zip_file,extlibs_folder)
		end
	end
end

function _M.solution(  )
	configurations { 'debug', 'release' }
	language 'c++'
	cppdialect "C++14"
	configuration 'release'
		optimize 'Speed'
		symbols 'Off'
		visibility 'Hidden'
	configuration 'debug'
		symbols 'On'
	configuration {}	
	filter{'system:macosx','gmake'}
	buildoptions { "-mmacosx-version-min=10.14" }
   	linkoptions  { "-mmacosx-version-min=10.14" }
   	filter{}

	sysincludedirs{
		path.join(_M.root,'build','include')
	}	
end

function _M.dependencies(  )
	for _,ext in ipairs(extlibs) do
		ext.lib(_M.root)
	end
end

function _M.lib(  )
	project 'llae-lib'
		kind 'StaticLib'
		targetdir 'lib'
		location 'build/project'

		_M.compile()
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,'src',c,'*.cpp'))
			table.insert(fls,path.join(_M.root,'src',c,'*.h'))
		end
		files(fls)

end

function _M.compile(  )
	sysincludedirs {
		path.join(_M.root, 'build','include'),
	}
	includedirs{
		path.join(_M.root, 'src') 
	}
end

function _M.exe(  )
	xcodebuildsettings{
   		MACOSX_DEPLOYMENT_TARGET='10.14'
   	}
   
	_M.compile()
	files {
		path.join(_M.root,'src','main.cpp'),
		path.join(_M.root,'src','bootstrap.cpp'),
	}
	filter {'configurations:release','toolset:gcc or clang'}
		postbuildcommands{
			(premake.tools.gcc.gccprefix or '') .. 'strip' .. ' %{cfg.targetdir}/%{cfg.targetname}'
		}
	filter{}
	_M.link()
end

function _M.link(  )
	links { 'llae-lib' }
	for _,ext in ipairs(extlibs) do
		ext.link()
	end
end

function _M.builddir( )
	local prj = configuration().name
	return path.getabsolute(path.join(_MAIN_SCRIPT_DIR, 'build', prj))
end




return _M
